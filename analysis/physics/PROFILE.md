# KSP 1.12.5 frame-time profile: where the time goes

Status: 2026-10-08. This profile was measured on the rebuilt `Assembly-CSharp.dll` (branch `profiling`). It is a companion to [PHYSICS-ENGINE-REVIEW.md](PHYSICS-ENGINE-REVIEW.md).

## Summary

**Measured on:** Ryzen AI 7 350 with integrated Radeon 860M; low graphics, 960×540. KSP is **CPU-bound** in every window measured.

**Per-tick cost scales linearly with part count:**
- about **16 µs per part per tick on the pad**
- about **30 µs per part per tick in powered ascent**
- on top of a fixed overhead of 0.7-2.1 ms

| Craft | Parts | Ascent FPS | Physics tick (CPU) |
|---|---|---|---|
| S | 21 | 217 | 2.9 ms |
| M2 | 153 | 115 | 6.4 ms |
| L2-AS | 457 | 26 | 15.8 ms |

**At 457 parts KSP cannot keep real time.** The physics tick (15.8 ms of CPU) plus about 10 ms of per-frame work pushes frames to 38 ms. That forces 1.74 ticks per frame, and game time runs at **0.91× wall time** (`PHYSICS_FRAME_DT_LIMIT = 0.04` caps it at 2 ticks per frame).

**Where the 457-part ascent tick goes:**

| Share | Component |
|---|---|
| 35% | floating-origin shift: 3.3 ms of script plus about 2.25 ms of extra PhysX on shift ticks |
| 21% | FlightIntegrator: aero, thermal, mass |
| 12% | PhysX baseline: joints and contacts for 457 bodies and 960 joints at 50 iterations |
| 11% | three per-part MonoBehaviours: Part, PartBuoyancy, CollisionEnhancer |
| 6% | PartModule FixedUpdates (mostly 49 engines) |
| 3% | SAS input |

**The joint solver is not the dominant cost at these sizes.**
- Doubling the joint count with autostruts (M2 168 → M2-AS 320 joints) added only 0.17 ms per tick of PhysX, and FPS did not change.
- The bulk is per-part managed bookkeeping and the every-tick world shift.

**GC is a hitch, not a throughput cost.**
- Allocation is about 0.3-1.4 MB/s; there are 0-3 full collections per minute.
- A frame with a collection takes 11-25 ms, against a median of 3-14 ms.

**Variance:**
- Repeats of M2 (run2 vs run4) and L2-AS (run3 vs run4) agree within 1% (M2 ascent FPS 114.8 vs 115.2) and 6% (L2-AS pad FPS 88.0 vs 83.1).
- The 21-part craft's ascent varies more, 173 vs 217 FPS, because its frames are tiny and dominated by terrain, UI and rendering.

## 1. Headline numbers per craft and window

Window lengths: pad 20 s, ascent 30 s wall time. All runs are from batch run4, except L2-AS which is also shown from run3 as a repeat.

| Craft | Window | Parts | PhysX bodies | Joints (attach CJ / all) | Colliders (vessel) | FPS | Frame ms mean | p95 | p99 | Physics ticks per frame | Sim rate (UT / wall) | GC per min | Alloc MB/s (lower bound) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| S | pad | 21 | 21 | 24 / 25 | 22 | 270.0 | 3.70 | 5.58 | 7.27 | 0.185 | 1.000 | 3 | 1.33 |
| S | ascent | 21 | 21 | 24 / 24 | 22 | 216.7 | 4.61 | 8.20 | 11.49 | 0.231 | 1.000 | 2 | 1.24 |
| S-AS | pad | 21 | 21 | 24 / 45 | 22 | 276.6 | 3.62 | 5.35 | 6.88 | 0.181 | 1.000 | 3 | 1.36 |
| S-AS | ascent | 21 | 21 | 24 / 44 | 22 | 190.9 | 5.24 | 9.01 | 11.65 | 0.262 | 1.000 | 2 | 1.09 |
| M2 | pad | 153 | 153 | 168 / 169 | 154 | 194.9 | 5.13 | 8.51 | 11.94 | 0.257 | 1.000 | 0 | 1.01 |
| M2 | ascent | 153 | 153 | 168 / 168 | 154 | 115.2 | 8.68 | 15.87 | 19.73 | 0.434 | 1.000 | 2 | 0.81 |
| M2-AS | pad | 153 | 153 | 168 / 321 | 154 | 195.3 | 5.12 | 8.52 | 11.08 | 0.256 | 1.000 | 0 | 0.99 |
| M2-AS | ascent | 153 | 153 | 168 / 320 | 154 | 112.8 | 8.86 | 16.07 | 19.59 | 0.443 | 1.000 | 2 | 0.78 |
| L2-AS | pad | 457 | 457 | 504 / 961 | 458 | 83.1 | 12.04 | 18.36 | 24.96 | 0.602 | 1.000 | 3 | 0.29 |
| L2-AS | ascent | 457 | 457 | 504 / 960 | 458 | **26.2** | **38.15** | 54.47 | 67.01 | **1.739** | **0.912** | 0 | 0.72 |
| L2-AS (run3) | pad | 457 | 457 | 504 / 961 | 458 | 88.0 | 11.37 | 16.96 | 22.80 | 0.569 | 1.000 | 3 | 0.32 |
| L2-AS (run3) | ascent | 457 | 457 | 504 / 960 | 458 | 26.0 | 38.38 | 54.79 | 66.83 | 1.769 | 0.921 | 0 | 0.69 |

**End state of the ascent windows:**

| Craft | Altitude | Speed |
|---|---|---|
| S | 2.9 km | 260 m/s |
| M2 | 6.1 km | 384 m/s |
| L2-AS | 2.6 km | 177 m/s |

Krakensbane was engaged in all three. Run-1 results for the original 150- and 446-part layouts are in `raw/tables-run1-run2.md`.

## 2. Frame breakdown by Unity player-loop phase

Values are ms per rendered frame, with the share of the frame in brackets.

| Phase | S pad | S ascent | M2 pad | M2 ascent | L2-AS pad | L2-AS ascent |
|---|---|---|---|---|---|---|
| FixedUpdate scripts (all physics-tick C#) | 0.115 (3%) | 0.261 (6%) | 0.604 (12%) | 1.847 (21%) | 3.780 (31%) | 20.486 (54%) |
| PhysX step (`PhysicsFixedUpdate`, incl. collision callbacks) | 0.093 (2%) | 0.398 (9%) | 0.205 (4%) | 0.950 (11%) | 1.187 (10%) | 6.957 (18%) |
| Update scripts | 0.652 (18%) | 0.720 (16%) | 1.234 (24%) | 1.523 (18%) | 3.021 (25%) | 4.180 (11%) |
| Rendering (FinishFrameRendering, UpdateAllRenderers, present) | 0.891 (24%) | 1.178 (26%) | 0.989 (19%) | 1.653 (19%) | 1.307 (11%) | 2.736 (7%) |
| LateUpdate scripts | 0.589 (16%) | 0.620 (13%) | 0.692 (13%) | 0.846 (10%) | 0.997 (8%) | 1.228 (3%) |
| Coroutines (≈ PQS terrain) | 0.825 (22%) | 0.776 (17%) | 0.820 (16%) | 0.940 (11%) | 0.885 (7%) | 0.918 (2%) |
| UI canvases (uGUI) | 0.193 (5%) | 0.281 (6%) | 0.231 (4%) | 0.381 (4%) | 0.385 (3%) | 0.635 (2%) |
| Particles, animation, audio, other engine | 0.32 (9%) | 0.35 (8%) | 0.33 (6%) | 0.48 (6%) | 0.44 (4%) | 0.79 (2%) |
| Outside player loop (frame wait, vsync) | 0.027 (1%) | 0.028 (1%) | 0.027 (1%) | 0.029 (0%) | 0.027 (0%) | 0.031 (0%) |
| **Total** | **3.70** | **4.62** | **5.13** | **8.68** | **12.03** | **38.18** |

The full 118-slot breakdown is in `raw/run4/*_slots.csv`.

## 3. The physics tick: components per tick

Values are ms per 0.02 s tick, inclusive. The share is of the ascent tick.

| Component | S pad | S ascent | M2 pad | M2 ascent | L2-AS pad | L2-AS ascent | L2-AS ascent share |
|---|---|---|---|---|---|---|---|
| Planetarium.FixedUpdate (UT, all body orbits) | 0.076 | 0.073 | 0.071 | 0.077 | 0.071 | 0.081 | 0.5% |
| VesselPrecalculate.FixedUpdate | 0.019 | 0.019 | 0.017 | 0.023 | 0.031 | 0.356 | 2.3% |
| CollisionEnhancer.FixedUpdate (×parts) | 0.028 | 0.032 | 0.184 | 0.187 | 0.504 | 0.419 | 2.7% |
| FlightInputHandler.FixedUpdate (SAS/autopilot, fly-by-wire) | 0.012 | 0.047 | 0.049 | 0.173 | 0.123 | 0.444 | 2.8% |
| Part.FixedUpdate (×parts, incl. OnFixedUpdate) | 0.062 | 0.060 | 0.251 | 0.271 | 0.708 | 0.710 | 4.5% |
| PartModule Unity FixedUpdates (engines etc.) | 0.026 | 0.156 | 0.060 | 0.416 | 0.133 | 0.969 | 6.1% |
| PartBuoyancy.FixedUpdate (×parts, on dry land) | 0.039 | 0.038 | 0.242 | 0.248 | 0.681 | 0.663 | 4.2% |
| **FlightIntegrator.FixedUpdate** | 0.214 | 0.199 | 1.207 | 1.167 | 3.448 | 3.293 | **20.9%** |
| · UpdateMassStats | 0.074 | 0.044 | 0.353 | 0.255 | 1.017 | 0.846 | 5.4% |
| · DragCubeSetupAndPartAeroStats | 0.006 | 0.005 | 0.061 | 0.057 | 0.174 | 0.155 | 1.0% |
| · UpdateThermodynamics | 0.040 | 0.055 | 0.242 | 0.269 | 0.630 | 0.650 | 4.1% |
| ·· of which UpdateConduction | 0.013 | 0.031 | 0.109 | 0.132 | 0.268 | 0.288 | 1.8% |
| · Integrate (force flush to PhysX, self) | 0.021 | 0.019 | 0.143 | 0.155 | 0.409 | 0.435 | 2.8% |
| · UpdateAerodynamics (drag + body lift) | 0.063 | 0.066 | 0.397 | 0.418 | 1.205 | 1.193 | 7.6% |
| Krakensbane.FixedUpdate | 0.003 | 0.008 | 0.003 | 0.034 | 0.003 | 0.093 | 0.6% |
| **FloatingOrigin.FixedUpdate (incl. setOffset)** | 0.001 | 0.347 | 0.001 | 1.240 | 0.001 | 3.324 | **21.1%** |
| Vessel, OrbitDriver, PQS, CollisionManager FixedUpdates | 0.011 | 0.011 | 0.011 | 0.017 | 0.014 | 0.036 | 0.2% |
| Not attributed above (other callbacks, hooks, second-tick precalc) | 0.13 | 0.14 | 0.26 | 0.40 | 0.56 | 1.39 | 8.8% |
| **All FixedUpdate scripts (player-loop slot)** | 0.620 | 1.130 | 2.353 | 4.256 | 6.280 | 11.779 | 74.6% |
| **PhysX simulate** | 0.498 | 1.725 | 0.798 | 2.190 | 1.969 | 4.000 | 25.3% |
| OnCollision*/OnJointBreak callbacks (inside the PhysX slot) | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 | 0% |
| **Physics tick total** | **1.12** | **2.86** | **3.15** | **6.45** | **8.25** | **15.78** | 100% |

**Per-frame physics-adjacent work** (Update phase, ms per rendered frame):

| Component | S pad | S ascent | M2 pad | M2 ascent | L2-AS pad | L2-AS ascent |
|---|---|---|---|---|---|---|
| VesselPrecalculate.Update (CoM/MOI stats, orbit rebuild from rigidbodies) | 0.014 | 0.016 | 0.057 | 0.103 | 0.368 | 0.731 |
| CalculatePhysicsStats, all calls (Update + 2nd tick) | 0.007 | 0.008 | 0.047 | 0.085 | 0.342 | 1.209 |
| FlightIntegrator.Update (sun raycast, occlusion) | 0.044 | 0.043 | 0.091 | 0.131 | 0.376 | 0.909 |
| Part.Update ×parts (incl. OnUpdate) | 0.059 | 0.053 | 0.255 | 0.266 | 0.748 | 0.753 |
| PQS.UpdateQuads (terrain coroutine) | 0.607 | 0.570 | 0.605 | 0.687 | 0.641 | 0.656 |
| PQS.UpdateVisual + mod hooks | 0.150 | 0.142 | 0.147 | 0.175 | 0.167 | 0.173 |
| KSP/UI/* scripts (self time) | 0.267 | 0.330 | 0.298 | 0.421 | 0.379 | 0.529 |

**PartModule work per tick** (L2-AS ascent; per-type detail in `raw/tables-main.md` table F):
- `ModuleEngines.FixedUpdate`: 0.78 ms for 49 engines (16 µs each). It was 0.33 ms for 17 engines in M2 and 0.12 ms for 3 in S.
- Every other module type is at most 0.06 ms per tick.
- `OnFixedUpdate` (the `PartModule` virtual) only runs on ACTIVE (staged) parts (`Assembly-CSharp/Part.cs:4161-4170`). It is negligible here.

**Unity physics step.** It is fully captured by the `FixedUpdate.PhysicsFixedUpdate` slot.
- Collision callbacks inside it were measured separately and are negligible: `Part.OnCollisionStay` etc. on the pad came to under 0.003 ms per tick.
- PhysX does not run on frames with zero ticks.

## 4. Floating-origin shifts double the PhysX step

KSP shifts the world on every tick while Krakensbane holds a frame velocity (`Assembly-CSharp/FloatingOrigin.cs:146-150`). The table splits the ascent windows by whether a tick contained a shift. Frames with mixed counts are excluded. Data is per-frame `fo_shifts` from run3 and later.

| Run | Ticks without shift | PhysX ms/tick | Scripts ms/tick | Ticks with shift | PhysX ms/tick | Scripts ms/tick |
|---|---|---|---|---|---|---|
| M2 | 81 | 0.71 | 3.05 | 1419 | **2.27** | 4.33 |
| M2-AS | 81 | 0.90 | 2.96 | 1419 | **2.47** | 4.36 |
| L2-AS (run4) | 90 | 1.89 | 8.82 | 1277 | **4.14** | 11.97 |
| L2-AS (run3) | 90 | 2.01 | 9.83 | 1293 | **4.10** | 11.82 |
| S | (every ascent tick shifted) | (pad: 0.50) | | 1500 | 1.73 | 1.13 |

**What the shift costs.**
- **+1.2 to +2.3 ms of PhysX per shifted tick**, on top of the `FloatingOrigin.FixedUpdate` script cost: 0.35 / 1.24 / 3.32 ms for S / M2 / L2-AS.
- The PhysX penalty has a fixed part (about 1.2 ms even for 21 bodies) and a part that grows with the vessel.
- That is consistent with the shift moving every collider in the scene. The scene census in `rt_scene.*` counts 1,325 static colliders (917 static MeshColliders, mostly PQS terrain quads and KSC) plus the vessel's dynamic bodies.
- PhysX then has to re-sync all of them (`autoSyncTransforms = True` [runtime]).
- The mechanism inside PhysX is inferred; the cost difference is measured.

## 5. Garbage collection and allocation

- **Managed heap:** 640-765 MB.
- **Allocation:** 0.3-1.4 MB/s in steady flight (lower bound).
- **Full collections:** 0-3 per minute.
- **Frames that contain a collection:**

| Run | Median frame | GC frames |
|---|---|---|
| S pad / ascent | 3.4 / 3.7 ms | 11-12 ms |
| M2 ascent | 6.0 ms | 12-20 ms |
| L2-AS pad | 14 ms | 23-25 ms |

- During the run-1 446-part self-destruction (explosions and debris), allocation rose to 20 MB/s and collections to 14 per minute, with GC frames of 39-92 ms (`raw/tables-run1-run2.md` table I).

GC is a periodic hitch of about +8-12 ms, not a sustained throughput cost, in steady flight.

## 6. Runtime constants

These values live in Unity scenes or engine settings, so the decompiled C# cannot show them. They were read live from `rt_*` keys in `raw/run4/*_summary.csv`.

| Constant | Value | C# initializer or default |
|---|---|---|
| `Physics.defaultSolverIterations` / velocity iterations | **50** / 1 | Unity default 6 / 1 |
| `Physics.autoSyncTransforms` | **True** | — |
| `Physics.gravity` | (0, 0, 0) | — |
| `Physics.defaultContactOffset`, `bounceThreshold`, `sleepThreshold` | 0.01, 0.2, 0.005 | — |
| Part `Rigidbody`: interpolation, collision detection, `maxDepenetrationVelocity`, `maxAngularVelocity` | None, Discrete, **1e32**, 50 | — |
| `FlightGlobals.StackAttachStiffNess` / `SrfAttachStiffNess` | **10 / 10** | 1 / 1 (`Assembly-CSharp/FlightGlobals.cs:28-30`) |
| Sample size-2 attach joint (Rockomax → probe) | 3 joints; angular spring 90,000; linear spring 1e20; dampers 0; `projectionMode` None; `enablePreprocessing` True; break force and torque 2200 | — |
| `Time.fixedDeltaTime` / `Time.maximumDeltaTime` | 0.02 / 0.04 | — |
| Krakensbane `MaxV` | 5 m/s | 5 |
| FloatingOrigin `threshold` / `velForContinuous` | 500 m / 5000 m/s | same |
| Kerbin `inverseRotThresholdAltitude` | **100,000 m** | 15,000 (`Assembly-CSharp/CelestialBody.cs:243`) |
| `TimeWarp.warpRates` | 1, 5, 10, 50, 100, 1000, 10000, 100000 | scene-serialized |
| `TimeWarp.physicsWarpRates` | 1, 2, 3, 4 | scene-serialized |
| `TimeWarp.maxPhysicsRate_index` | 0, so MaxPhysicsRate = 1× | scene-serialized |
| `TimeWarp.altitudeLimits` | 0, 0.05, 0.05, 0.1, 0.2, 0.4, 0.8, 1 | scene-serialized |
| Scene colliders during the L2-AS flight | 1,325 static (917 static MeshColliders), 458 dynamic, 0 kinematic | — |

## 7. Scaling curve

Physics tick cost (ms per 0.02 s tick, scripts plus PhysX) against part count:

| Parts | Pad | Ascent |
|---|---|---|
| 21 | 1.12 | 2.86 |
| 153 | 3.15 | 6.45 |
| 457 | 8.25 | 15.78 |

- A least-squares line gives **pad ≈ 0.7 ms + 16.4 µs × parts** and **ascent ≈ 2.1 ms + 29.8 µs × parts** (three points; every point within 5% of the line).
- **Frame time:**
  - Pad: 3.7 → 5.1 → 12.0 ms.
  - Ascent: 4.6 → 8.7 → 38.2 ms.
- Above about 20 ms per frame the 0.02 s tick has to run more than once per frame. Frame time then grows superlinearly: at 457 parts, 1.74 ticks × 15.8 ms is spent in physics.
- **Real-time break-even.** In steady state, real time needs `frame = other / (1 − tick/20 ms)` to stay within 40 ms (2 ticks). With the measured 10.7 ms of non-physics frame work at 457 parts, that requires `tick ≤ 14.6 ms`. The ascent line reaches that at **about 420 parts** on this machine. The measured sim rate at 457 parts is 0.91, consistent with this.
- **Per-part, per-tick unit costs** (L2-AS pad):

| Component | µs per part per tick |
|---|---|
| FlightIntegrator | 7.5 |
| UpdateAerodynamics | 2.6 |
| UpdateMassStats | 2.2 |
| thermal | 1.4 |
| Integrate | 0.9 |
| Part.FixedUpdate | 1.5 |
| PartBuoyancy | 1.5 |
| CollisionEnhancer | 1.1 |
| PhysX (pad, 960 joints) | 4.3 |
| Part.Update (per frame) | 1.6 |

- Ascent adds, per part per tick: about 7 µs of FloatingOrigin script (particles and transforms), about 5 µs of extra PhysX re-sync, and about 2 µs of engine modules.

## 8. GPU-bound or CPU-bound

**Verdict: CPU-bound in every configuration measured. The integrated GPU does not limit frame rate.**

**Evidence.**
- With vsync off and no frame cap, the time spent outside the player loop and in `PostLateUpdate.PresentAfterDraw` is 0.03-0.04 ms per frame in every run. The main thread never waits on present.
- `SystemInfo.graphicsMultiThreaded = False` [runtime], so Direct3D 11 submission happens on the main thread inside the Rendering phase. That phase is 0.9-2.7 ms at low settings, 7-26% of the frame.
- **GPU-sensitivity run** (`run5-M2-gfxdefault`). The 153-part M2 was re-flown with the user's own `settings.cfg` graphics and the resolution raised from 960×540 to 1280×720. Vsync stayed off and the cap at 1000.
  - Graphics settings used: `QUALITY_PRESET 5`, `ANTI_ALIASING 2`, `SHADOWS_QUALITY 4`, `LIGHT_QUALITY 8`, `TEXTURE_QUALITY 1`, `AERO_FX_QUALITY 3`, `SURFACE_FX True`.

| M2 | Low, 960×540 (run4) | User default, 1280×720 (run5) |
|---|---|---|
| Pad FPS / frame ms | 194.9 / 5.13 | 140.7 / 7.11 |
| Ascent FPS / frame ms | 115.2 / 8.68 | 85.7 / 11.67 |
| Rendering phase, ascent (ms/frame) | 1.65 | 2.73 |
| Physics ticks per frame, ascent | 0.43 | 0.58 |
| FixedUpdate scripts + PhysX, ascent (ms/frame) | 2.80 | 4.03 |
| Outside player loop (ms/frame) | 0.03 | 0.04 |

**Interpretation.**
- Higher settings add about 1.1 ms of rendering per frame. That is shadow cascades and more draw calls, CPU-side as far as can be told; no present wait appears.
- They also add about 1.2 ms of physics per frame. Per-tick physics cost did not change; longer frames simply contain more ticks.
- On this laptop, graphics settings move the frame by about a third. The scaling with part count, which is CPU physics, moves it by about 4× (pad) to 8× (ascent) between 21 and 457 parts.
- **Not measured:** GPU execution time itself. No GPU timer queries are possible from a release player. If a GPU stall were hidden inside `FinishFrameRendering`, it would be at most the 1.1 ms difference above.

## 9. What could and could not be measured

**Measured directly:**
- Every Unity player-loop phase, including the PhysX step as a whole.
- All 608 KSP Unity callbacks, plus FlightIntegrator, VesselPrecalculate, PQS and FloatingOrigin sub-phases.
- Per-PartModule-type `OnFixedUpdate` and `OnUpdate`.
- Physics ticks per frame, sim rate, GC count, heap size, joint/body/collider counts.
- Script execution order (one-frame trace) and scene-serialized constants.

**Not measured:**
- **Inside PhysX.** The broadphase, narrowphase, solver and pose-sync split is not observable from a release Unity player: `Profiler.Recorder` needs a development build. The floating-origin attribution in §4 is by correlation.
- **Render-thread and GPU time.** `graphicsMultiThreaded = False` [runtime], so D3D11 submission is on the main thread and inside the Rendering phase. GPU execution time itself was not read. §8 bounds it indirectly.
- **Exact allocation rate.** Mono's Boehm GC exposes no allocation counter. The figure is a lower bound from heap deltas.
- **Staging, decoupling and joint-break events** (CollisionManager O(C²) rebuild, autostrut rebuild, new-vessel creation). These are one-off spikes; no event fell inside a measurement window.
- **Other situations:** multiple loaded vessels, debris, docking, wheels, water, re-entry heating, physics warp and orbit. Only pad and low-atmosphere powered ascent were profiled.
- **Variance:** two repeats per configuration at most.
- **Comparison with the shipped DLL.** All numbers come from the rebuilt `Assembly-CSharp` (`/optimize+`, with instrumentation costing 0.6-2.1% per frame per table G). The shipped binary was not profiled. With no instrumentation hooks it can only be measured externally, e.g. by frame time.

## 10. Files

**Generated tables:**
- `raw/tables-main.md`: runs 3 and 4. Tables A-I, including top-20 methods per window and PartModule per-type costs.
- `raw/tables-run1-run2.md`: runs 1 and 2, the first layouts.

**Raw CSVs:** in `raw/run{1..5}/`. For each `<tag>`:

| File | Contents |
|---|---|
| `<tag>_summary.csv` | key-value per window, including `rt_*` constants |
| `<tag>_<window>_slots.csv` | player-loop slots |
| `<tag>_<window>_methods.csv` | instrumented methods |
| `<tag>_<window>_frames.csv` | per-frame samples |
| `<tag>_calltrace.csv` | one-frame call order |

**Logs:** `decompiled/logs/ksp-prof-<tag>-{player,KSP}.log`.

**Scripts:**
- `analysis/physics/scripts/gen_craft.py` (craft)
- `analysis/physics/scripts/analyze.py` (tables)
- `decompiled/tools/scripts/{physprof_instrument.py, physprof_manual.py, physprof_manual2.py, profile_ksp.ps1}`

**Re-running.** Check out branch `profiling`, or add a worktree. Run the three instrumentation scripts only on a fresh checkout of main; the branch already contains their output. Then:
1. Build with `-p:Optimize=true`.
2. Copy the DLL and PDB over `KSP-Decompiled/GameData/Assembly-CSharp-cleaned/`.
3. Apply the low-graphics settings: `decompiled/backup/settings.cfg.profiling-low`.
4. Run `profile_ksp.ps1 -Tag <t> -Craft <path>`.
5. Restore `settings.cfg.pre-profiling` and rebuild main afterwards.

**Extending the driver.** It is a linear coroutine of stages driven by command-line switches: new game → launch → settle → windows → write → quit.
- A later capture mode, e.g. FloatCurve or Unity `Random` dumps for the harness, can be added as another `-physprof-mode` value.
- That mode would read a cfg list of actions and output CSV paths and reuse the same `PhysProf.Info`/CSV writer.
- Nothing like it is implemented yet.

## Method

### Instrumentation

All of it lives on branch `profiling` of `decompiled/KSP-Decompiled`. The shipped `main` branch is clean.

- **`PhysProf`** (`Assembly-CSharp/PhysProf/PhysProf.cs`) is a static accumulator.
  - `Begin()/End(id)` read `Stopwatch.GetTimestamp` and keep inclusive and self time. Self time is computed with a parent stack, so nested instrumented calls are subtracted.
  - Measured cost: **~50 ns per Begin/End pair** (calibrated at startup, `calib_pair_ns`). With 600-8,200 instrumented calls per frame, the estimated overhead is **0.6-2.1% of frame time** (`raw/tables-main.md` table G).
- **Auto-instrumentation** (`decompiled/tools/scripts/physprof_instrument.py`) wraps every Unity callback in try/finally Begin/End. It covers `FixedUpdate`, `Update`, `LateUpdate`, `OnCollisionEnter/Stay/Exit` and `OnJointBreak`: **608 methods in 490 files**. Embedded third-party code is excluded: TMPro, VehiclePhysics, EdyCommonTools, Smooth, SoftMasking, UniLinq, LibNoise and Highlighting.
- **Manual sub-phases** (`physprof_manual.py`, `physprof_manual2.py`):
  - FlightIntegrator: UpdateMassStats, DragCubeSetupAndPartAeroStats, CheckThermalGraph, UpdateOcclusion, UpdateThermodynamics, ThermalIntegrationPass, UpdateConduction, Integrate (root call) and UpdateAerodynamics (per part).
  - VesselPrecalculate: MainPhysics, CalculatePhysicsStats, CalculateGravity.
  - `OrbitDriver.UpdateOrbit`, `FloatingOrigin.setOffset`, and Krakensbane `AddExcess` / `Zero`.
  - `Part.ModulesOnFixedUpdate/OnUpdate`, plus **per-PartModule-type** `OnFixedUpdate`/`OnUpdate`. These use dynamic ids cached on each module.
  - The PQS terrain coroutine: `Mod_OnPreUpdate`, `UpdateVisual`, `UpdateQuads`, `Mod_OnUpdateFinished`.
- **Unity PlayerLoop markers.** `PlayerLoop.SetPlayerLoop` gets a marker system inserted before every subsystem of every top-level group: 118 slots in 7 groups. This times every engine phase, including:
  - `FixedUpdate.PhysicsFixedUpdate`, i.e. the PhysX step plus the collision-callback dispatch;
  - rendering (`PostLateUpdate.FinishFrameRendering`);
  - coroutines;
  - the gap outside the player loop.

  The markers also count FixedUpdates per frame.
- **Per frame** it records frame ms, the number of fixed ticks, the main slot times, `GC.CollectionCount(0)`, `GC.GetTotalMemory(false)` and the number of floating-origin shifts.
  - Allocation rate is a **lower bound**: the sum of positive heap deltas between frames that had no collection.
- **One-frame call-order trace.** Halfway through the ascent window, every top-level instrumented callback and every player-loop marker of one frame is recorded in execution order (`*_calltrace.csv`). This is what fixes Unity's script execution order, which is not in the C#.
- **Runtime constant dump** (`rt_*` keys in `*_summary.csv`). These are scene-serialized and engine values that the decompiled C# cannot show:
  - PhysX solver settings
  - joint and Rigidbody settings of a sample attach joint
  - Krakensbane and FloatingOrigin thresholds
  - warp rate tables
  - attach stiffness multipliers
  - a census of the scene's colliders

### Automation

`PhysProfDriver` (`Assembly-CSharp/PhysProf/PhysProfDriver.cs`) is a `KSPAddon(MainMenu)`. It does nothing unless `-physprof-craft <path>` is on the command line. When it is:
1. It creates a sandbox save named `physprofile` (`GamePersistence.CreateNewGame` → `Game.Start` → Space Center).
2. It launches the craft on the launch pad with `FlightDriver.StartWithNewLaunch`, which is the same call the Launch button makes.
3. It waits for the vessel to unpack, then settles for 10 s.
4. **pad window:** 20 s, landed on the pad, engines off.
5. It stages through `StageManager.ActivateNextStage` (the real space-bar path), sets throttle to 1 (`FlightInputHandler.state` plus an `OnFlyByWire` hook), turns SAS on and waits 3 s.
6. **ascent window:** 30 s of powered ascent. Krakensbane is engaged, there are aero forces, and the floating origin shifts every tick.
7. It writes the CSVs and calls `Application.Quit()`.

`decompiled/tools/scripts/profile_ksp.ps1` launches `KSP-dev` with `-popupwindow -screen-width 960 -screen-height 540` and the driver arguments. It waits for exit, killing the process on timeout, and copies the logs to `decompiled/logs/ksp-prof-<tag>-*.log`.

### Graphics settings

`KSP-dev/settings.cfg` was modified for the runs and backed up to `decompiled/backup/settings.cfg.pre-profiling`. It is restored after the runs.

| Setting | Value |
|---|---|
| `QUALITY_PRESET` | 0 |
| `ANTI_ALIASING` | 0 |
| `TEXTURE_QUALITY` | 2 |
| `SHADOWS_QUALITY` | 0 |
| `LIGHT_QUALITY` | 1 |
| `AERO_FX_QUALITY` | 0 |
| `SURFACE_FX` | False |
| `CELESTIAL_BODIES_CAST_SHADOWS` | False |
| `SYNC_VBL` | 0 |
| `FRAMERATE_LIMIT` | 1000 |
| Resolution | 960×540 windowed |

- The terrain preset was left at Default, so PQS CPU cost is what a player sees.
- `PHYSICS_FRAME_DT_LIMIT` was left at 0.04.

### Build and machine

- **Build:** MSBuild Debug configuration with `-p:Optimize=true` (csc `/optimize+`).
  - The default `build.sh` Debug build is `/optimize-`, which would have penalised the managed code compared with the shipped game.
- **Machine** (read by the driver):
  - AMD Ryzen AI 7 350, 16 threads
  - AMD Radeon 860M integrated GPU, Direct3D 11, `graphicsMultiThreaded = False`
  - 24 GB RAM
  - Windows 11

### Test craft

The craft are in `analysis/physics/craft/`; the generator is `analysis/physics/scripts/gen_craft.py`. All are stock parts only, and every size uses the same booster design:
- **Core:** RC-L01 probe (root) / K × X200-32 / Mainsail.
- **Boosters:** rings of 4 boosters. Each booster is an HDM radial decoupler, 6 × FL-T100, an LV-T45 and a Type-B nose cone.
- **Staging:** all engines are in the first stage.

| Craft file | Parts | Core tanks K | Booster rows × per row | Engines | Autostrut | Height (part centres) | Notes |
|---|---|---|---|---|---|---|---|
| `PhysProf-S.craft` | 21 | 1 | 1 × 2 | 3 | Off | 6 m | |
| `PhysProf-S-AS.craft` | 21 | 1 | 1 × 2 | 3 | Grandparent on every non-root part | 6 m | |
| `PhysProf-M2.craft` | 153 | 7 | 4 × 4 | 17 | Off | 28 m | |
| `PhysProf-M2-AS.craft` | 153 | 7 | 4 × 4 | 17 | Grandparent | 28 m | |
| `PhysProf-L2-AS.craft` | 457 | 23 | 12 × 4 | 49 | Grandparent | 88 m | ~680 t |
| `PhysProf-L2.craft` | 457 | 23 | 12 × 4 | 49 | Off | 88 m | **collapsed within 1 s of unpacking**, unusable |
| `run1-layout/PhysProf-M.craft` | 150 | 4 | 4 × 4 | 17 | Off | | run 1 only |
| `run1-layout/PhysProf-L.craft` | 446 | 12 | 12 × 4 | 49 | Off | | run 1 only; pad window valid, ascent window invalid |

**Row layout.**
- Booster rows sit on every other core tank. Successive rows are rotated by 30°.
- This keeps every booster at least 10 m (the ModuleEngines `exhaustDamageMaxRange`) away from the exhaust path of the booster above it.

**Why run 1's 446-part craft failed.** It had rows on every tank and only two phases. That put nose cones 0.7 m under the engines of the row above.
- Exhaust impingement applies the full engine thrust as a force on the hit part, plus heat (`Assembly-CSharp/ModuleEngines.cs:1302-1345`).
- 211 parts exploded within seconds of ignition.
- Its ascent window is therefore not a valid measurement and is excluded from the scaling tables.

**Joint counts** were measured at runtime by summing `Joint` components on every part GameObject:

| Craft | Attach ConfigurableJoints | All joints | Notes |
|---|---|---|---|
| S | 24 | 25 | the extra joint is the landed-vessel FixedJoint anchor on the pad |
| M2 | 168 | 169 | |
| L2-AS | 504 | 961 | autostruts add one joint per non-root part |
