# Physics spike (Jolt): what WP-19 / WP-20 / WP-12 should build

Status: 2026-10-11, branch `jolt-spike` (from main `af3798c6`, main `a5d23742` merged; full suite 34/34 green). Spike code: `core/spikes/jolt/` (opt-in CMake target, never part of
`run_tests.ps1` / CI). Raw results: `core/spikes/jolt/results/*.md|csv`. Progress log: `core/PROGRESS-jolt.md`.
Inputs: [PHYSICS-ENGINE-REVIEW.md](../analysis/physics/PHYSICS-ENGINE-REVIEW.md), [PROFILE.md](../analysis/physics/PROFILE.md),
[TIERING.md](../analysis/tiering/TIERING.md). C# citations are relative to `decompiled/KSP-Decompiled/Assembly-CSharp/`.

## Summary

**Recommendation: WP-20 builds the reduced rigid-body model (rigid clusters + structural load graph) on Jolt Physics v5.6.0
inside kspcore, with double-precision positions in KSP's own body-fixed / inertial frame and no per-tick world shift. The
PhysX-equivalent articulated model (one body per part, PartJoint springs) is kept only as a test oracle.**

What the spike measured on this machine (Ryzen 9 7950X3D, one thread; KSP's numbers are PROFILE.md's, taken on another CPU,
and KSP's tick does more work than the spike's: no thermal, SAS, module logic or buoyancy here, so compare the component rows):

| | KSP 1.12.5 (PROFILE.md) | Jolt, articulated (PhysX-equivalent) | Jolt, reduced (recommended) |
|---|---|---|---|
| 457-part L2-AS ascent, whole tick | 15.8 ms | 2.8 ms | **0.33 ms** |
| of which rigid-body solver | 4.0 ms (1.9-2.25 without the shift) | 2.5 ms (50 velocity steps) | **0.004 ms** |
| of which world shift | 3.3 ms script + ~2.25 ms PhysX | 0 | 0 |
| of which per-part pipeline (mass, atmosphere, aero, engines) | FlightIntegrator 3.3 ms | 0.3 ms (exact ports, native) | 0.3 ms (SoA aero kernel: 0.18 ms) |
| 2,053-part XL2-AS | -- | 14.7 ms (6.7 ms with 16 threads) | **1.5 ms** |
| iterations needed to hold the craft together | 50 PhysX [runtime] | >= 30 (10: the craft tears itself apart) | 10 (Jolt default; few or no joints) |
| physics warp, 30 s L2-AS ascent vs 1x | capped at 4x | -10 % at 4x, breaks at 8x | **within 1 % to 10-25x** |
| structural failure | PhysX joint break | joint impulse > capacity | load solve vs the same capacity: equal to the articulated impulses on tree craft (median 0.0 %) |

Every lever of PROFILE.md was prototyped:

1. **No per-tick world shift** (exact): Jolt `DOUBLE_PRECISION` positions in Kerbin's body-fixed frame (origin 600 km away) rest
   as quietly as single precision at a local origin and cost the same; single precision at the body centre is unusable (a vessel
   on the pad drifts 13 m in 10 s). Re-adding KSP's every-tick shift in Jolt would double the reduced model's tick. Orbital speed
   needs a Galilean frame velocity for jointed vessels (KSP's `Krakensbane.FrameVel` without the shift): orbit error 193 m ->
   23 m.
2. **Data-oriented per-part pipeline** (exact): kspcore's gated ports, run natively over a vessel-local float frame, take
   0.6-0.9 us per part per tick (KSP: FlightIntegrator alone 7.5 us); an SoA aero kernel, bit-identical on 4 million
   part-ticks, brings it to 0.4 us.
3. **Reduced rigid-body model** (approximate, wobble dropped as decided): solver cost independent of part count, 30 s ascents
   within 0.01-0.4 % of the articulated model (M2-AS, L2-AS, XL1-AS, XL2-AS; the 21-part S tumbles at Mach 1 in both models and
   diverges after 10 s), joint loads equal to the articulated impulses on tree craft, deterministic across runs and thread counts.

The remaining risk for WP-20 is load sharing in strutted craft (section 6, R1): the tree solve is conservative (~2x on
autostrutted stacks); the indeterminate graph solve prototyped here closes most of the gap (median force difference 73-76 % ->
~20 %, max load / capacity 0.039-0.046 against the articulated 0.033-0.035) at 0.02-0.08 ms per tick with a direct sparse
factorisation that is redone (0.1-0.6 ms) only when the structure or the set of ground contacts changes. Section 7 splits
WP-19 / WP-20 / WP-12 into nine implementer packages with their gates.

## Contents

1. [What the spike is](#1-what-the-spike-is)
2. [Measurements](#2-measurements)
3. [Recommendation for WP-20](#3-recommendation-for-wp-20)
4. [Exact / approximate / changed, per optimisation](#4-exact--approximate--changed-per-optimisation)
5. [The interface WP-19 / WP-20 / WP-12 need from kspcore](#5-the-interface-wp-19--wp-20--wp-12-need-from-kspcore)
6. [Risks](#6-risks)
7. [Package split and gates](#7-package-split-and-gates)
8. [Reproducing](#8-reproducing)

## 1. What the spike is

Plain C++20 under `core/spikes/jolt/`, one executable `jolt_spike` linking kspcore and Jolt Physics **v5.6.0** (2026-07-11,
commit `e77f175`, fetched as the release archive pinned by SHA-256 `6e069ee0...3f8e`). It is opt-in: `KSPCORE_BUILD_SPIKES`
(default OFF) and two presets with their own build trees (`spike-jolt`: Jolt `DOUBLE_PRECISION`; `spike-jolt-f32`: single
precision). Nothing in kspcore, its tests or CI changed behaviour; the full suite is green with the spike on the branch.

| file | what it does |
|---|---|
| `src/ksp_context.*` | the flight-scene data through kspcore, as the gates use it: GameData mirror, PhysicsGlobals (Physics.cfg), Planetarium + the Kerbol system (WP-07/09), Kerbin's PQS height stack (WP-11), craft load through `ShipConstruct.LoadShip` (WP-04/18) + `AssembleVesselFromCraft` (WP-14/18) |
| `src/craft_model.*` | the physical description with KSP's rules: a body per FULL part (Part.cs:1868-1895), physicsless parts folded into their owner (FlightIntegrator.cs:700-713), `rb.mass` from kspcore's `Vessel::UpdateMassStats` (FlightIntegrator.cs:452-493), inertia = collider tensor per unit mass x max(1, m) (Part.cs:1889, :4263), every attach joint as `PartJoint.Create` builds it (PartJoint.cs:96-313: smaller node, 3 sub-joints on a ring for size >= 2, stiffness 1500 (size+1) {2, 0.8} x 10 [runtime], rigid flag, break force / torque incl. `JointBreakForceFactor` 50), autostruts (Part.cs:8098-8126, Grandparent / Root / Heaviest). Colliders are proxies: the box of each part's first drag cube |
| `src/articulated.*` | model A, the PhysX-equivalent: one Jolt body per FULL part, one `SixDOFConstraint` per PartJoint sub-joint (translation locked = PhysX's 1e20 linear drive; rotation free with position motors, stiffness = the angular drive spring, damping 0; rigid = locked), break by constraint impulse / dt > PhysX breakForce |
| `src/reduced.*` | model B, the reduced model: parts welded into one body per cluster (`reduced-whole`: one per connected vessel; `reduced-split`: pieces between decouplers joined by Jolt fixed constraints), mass / CoM / inertia summed from KSP's per-part rules, forces accumulated per part and applied once per cluster, structural loads per attach joint every tick (tree solve, O(parts)), break -> re-cluster |
| `src/structure_graph*.cpp` | model B's statically indeterminate solve: every sub-joint (attach and autostrut) and every ground contact an elastic element; PCG with 6x6 block-Jacobi (`structure_graph.cpp`) or a sparse block Cholesky factored per structural change (`structure_graph_direct.cpp`, the default) |
| `src/part_pipeline.*` | lever 2: the per-part tick on kspcore's exact ports (WP-14 `UpdateMassStats`, WP-09 atmosphere, WP-21 `DragCubeSetupAndPartAeroStats` + `UpdateAerodynamics`), part transforms written in a vessel-local float frame; an SoA aero kernel with the same float statements and a bit-for-bit comparison mode |
| `src/terrain_patch.*` | 900 static Jolt mesh quads (30 x 30 quads of 15 x 15 vertices, 50 m) around the LaunchPad from Kerbin's real PQS heights (WP-11), in the body-fixed double frame: KSP's flights had 917 static MeshColliders |
| `src/flight.*` | one scenario in KSP's measured tick order (PHYSICS-ENGINE-REVIEW.md 1.2): readback + gravity / Coriolis / centrifugal at the CoM with KSP's 16-tick easing, per-part work, force flush, Jolt step, loads / breaking; pad settle then full-thrust ascent with aero, or an orbit coast; per-phase timers |
| `src/bench.*`, `tools/run_benches.sh` | the measurement sets below (results in `results/`) and `bench selftest`: the checks section 7 proposes as gates (joint counts == runtime, SoA == exact, tree loads == articulated impulses, trajectories, determinism) |

Not in the spike (WP-20 must add them, section 6): thermal (WP-22 port not wired), engine logic beyond thrust (no
`ThrustUpdate`, no propellant drain), SAS / reaction wheels, buoyancy, `CollisionEnhancer`, wheels, the .mu colliders.
Coordinates are KSP's Unity axes used as they are, as PhysX does inside Unity (the dynamics formulas do not depend on the
handedness convention), so no conversion layer is needed.

## 2. Measurements

**Machine:** AMD Ryzen 9 7950X3D (16 cores / 32 threads, 64 GB), Windows 11, MSVC 14.44 (VS 2022 Community, the toolchain of
`run_tests.ps1`), Release (`/O2`; Jolt with its own flags `/O2 /Oi /Ot /fp:fast`, AVX2). Unless stated: one thread, the
process pinned to one core of the V-cache CCD at high priority (`--affinity 4`), best of 3 runs (`--repeat 3`). Unpinned runs are
30-60 % slower and noisier (the scheduler moves the thread between the two CCDs); the unpinned tables are kept in `results/`
too. Scenario: the craft from `analysis/physics/craft/` assembled on the LaunchPad of the real PQS terrain (900 static mesh
quads), 2 s settle, then 28 s at full throttle with drag-cube aerodynamics; "tick" is the mean over the 28 s.

The PROFILE.md numbers for KSP were taken on another machine (Ryzen AI 7 350 laptop) with a game tick that does more work than
the spike's (thermal, module logic, SAS, buoyancy, CollisionEnhancer, FX). Compare components, not whole ticks.

### 2.1 Craft extraction fidelity

`jolt_spike info` builds the physical description from the real craft through kspcore and gets the profile's **runtime** joint
counts (PROFILE.md "Joint counts", measured by summing Joint components in game) exactly:

| craft | parts | attach joints (3-sub-joint) | autostruts | sub-joints (spike) | ConfigurableJoints in game | mass (t) |
|---|---|---|---|---|---|---|
| PhysProf-S | 21 | 20 (2) | 0 | 24 | 24 | 35.2 |
| PhysProf-S-AS | 21 | 20 (2) | 20 | 44 | 44 (ascent) | 35.2 |
| PhysProf-M2 | 153 | 152 (8) | 0 | 168 | 168 | 218.1 |
| PhysProf-M2-AS | 153 | 152 (8) | 152 | 320 | 320 (ascent) | 218.1 |
| PhysProf-L2-AS | 457 | 456 (24) | 456 | 960 | 960 (ascent) | 677.3 (~680 in PROFILE.md) |
| PhysProf-XL1-AS / XL2-AS (generated, same design) | 1,027 / 2,053 | 1,026 / 2,052 | 1,026 / 2,052 | 2,160 / 4,320 | -- | 1,538 / 3,088 |

### 2.2 Tick cost vs part count (`results/scaling_ascent_pinned.md`)

ms per 0.02 s tick, powered ascent, one thread. Articulated at 50 Jolt velocity steps (section 2.3: it needs >= 30), reduced at
Jolt's default 10 / 2.

| craft | parts | articulated: tick / Jolt step | reduced-whole: tick / Jolt step | reduced-split: tick / Jolt step | per-part pipeline (all models) | KSP (PROFILE.md, other CPU): tick / PhysX |
|---|---|---|---|---|---|---|
| PhysProf-S | 21 | 0.077 / 0.063 | 0.017 / 0.002 | 0.020 / 0.005 | 0.013 | 2.86 / 1.73 |
| PhysProf-M2-AS | 153 | 0.86 / 0.75 | 0.110 / 0.003 | 0.130 / 0.020 | 0.10 | 6.45 (M2) / 2.19 |
| PhysProf-L2-AS | 457 | 2.83 / 2.51 | 0.330 / 0.004 | 0.387 / 0.060 | 0.29-0.31 | 15.78 / 4.00 (1.9-2.25 on ticks without a shift) |
| PhysProf-XL1-AS | 1,027 | 7.07 / 6.26 | 0.763 / 0.004 | 0.935 / 0.141 | 0.68-0.76 | -- |
| PhysProf-XL2-AS | 2,053 | 14.70 / 13.04 | 1.533 / 0.005 | 1.841 / 0.275 | 1.37-1.57 | -- |

- The reduced model's rigid-body step is **constant** (~4 us per vessel, one body); its tick is the per-part pipeline
  (0.6-0.9 us per part per tick on the exact ports, depending on the run) plus the structural loads (`after`: 0.04 ms at 457
  parts, tree solve).
- The articulated model's step grows linearly with sub-joints (2.6-3.0 us per sub-joint at 50 steps).
- Unpinned (`results/scaling_ascent.md`): the same shape, L2-AS articulated 3.9 ms, reduced 0.52 ms.

Pad (engines off, `results/scaling_pad.md`): the same costs; the articulated L2-AS / XL1-AS shed their boosters after ~25 s on the
pad and XL2-AS collapses at 6 s, the reduced model holds every craft (max load / capacity on the pad 0.13 / 0.29 / 0.59).

### 2.3 The articulated model needs PhysX-like iteration counts (`results/iterations.md`)

| craft | Jolt velocity / position steps | tick ms (unpinned) | joint stretch max (m) | joints broken | 30 s ascent |
|---|---|---|---|---|---|
| PhysProf-M2-AS | 4 / 2 | 0.42 | 1.11 | 59 | destroyed |
| | 10 / 2 (Jolt default) | 0.56 | 0.48 | 42 | destroyed |
| | 20 / 2 | 0.67 | 0.22 | 18 | lost boosters |
| | 30 / 2 | 0.88 | 0.0018 | 0 | 4,891 m |
| | 50 / 2 | 1.22 | 0.0012 | 0 | 4,909 m |
| PhysProf-L2-AS | 10 / 2 | 1.68 | 0.61 | 143 | destroyed |
| | 30 / 2 | 3.37 | 0.0027 | 0 | 3,029 m |
| | 50 / 2 | 4.89 | 0.0018 | 0 | 3,045 m |
| PhysProf-L2 (no autostruts) | 50 / 2 | 2.69 | 0.020 | 20 | core stack breaks at 15 s |

KSP runs PhysX at 50 position iterations [runtime] for the same reason. The unstrutted L2 that collapsed on the pad in KSP
(PROFILE.md) survives the pad here but breaks in the ascent: the same class of behaviour.

### 2.4 Threads (`results/threads.md`, unpinned)

| craft | model | 1 thread | 2 | 4 | 8 | 16 |
|---|---|---|---|---|---|---|
| L2-AS (960 sub-joints) | articulated: Jolt step | 3.46 | 3.84 | 3.50 | 3.46 | 4.02 |
| XL2-AS (4,320 sub-joints) | articulated: Jolt step | 27.8 | 11.8 | 8.3 | 6.9 | 6.7 |
| L2-AS / XL2-AS | reduced-whole: tick | 0.51 / 2.35 | 0.58 / 2.08 | 0.60 / 2.18 | 0.53 / 2.42 | 0.57 / 2.61 |

A vessel is one island; Jolt parallelises a single island only when it is very large (XL2-AS: 4.2x at 16 threads). The
reduced model has nothing to parallelise in the solver; its per-part pipeline (single-threaded in the spike) is the part that
would be split across cores, per vessel or per part range.

### 2.5 Physics warp beyond 4x (`results/warp.md`, unpinned)

The same 30 s of game time at dt = 0.02 x rate (KSP's physics warp: `Time.fixedDeltaTime = 0.02 * rate`, TimeWarp.cs:423),
compared with the 1x run of the same model:

| craft | model | 2x | 4x | 10x | 25x | 50x |
|---|---|---|---|---|---|---|
| L2-AS | articulated: altitude error / state | -2.0 % | -9.8 % | broke (14 joints) | -38 % | -146 % (fell back) |
| L2-AS | reduced-whole | -0.01 % | +0.38 % | -0.67 % | -1.1 % | -4.0 % |
| L2-AS | reduced-split (49 bodies) | -0.1 % | +0.02 % | -1.3 % | -3.4 % | -50 % (constraints fail at 1 s) |
| M2-AS | articulated | -0.6 % | -39 % | crashed | crashed | crashed |
| M2-AS | reduced-whole | +0.1 % | +0.2 % | -0.3 % | -6.3 % | -39 % |

- **Articulated: KSP's 4x cap is about right** (the joint chain at a larger dt degrades the trajectory and then breaks).
- **Reduced: physics warp is limited by integration accuracy, not stability**: within 1 % up to 10-25x on these ascents.
- Substepping instead of a larger dt (every tick at 0.02 s, several per frame) keeps 1x accuracy; one core sustains
  20 ms / tick-time: ~60x real time for L2-AS on the reduced model (0.33 ms ticks), ~7x for the articulated model.

### 2.6 Lever 1: no per-tick world shift (`results/shift.md`, `precision*.md`, `scaling_ascent_*padorigin.md`)

- **What the shift would cost in Jolt** (re-added: every tick, every vessel body and the 900 static quads move by -v dt):
  +0.10-0.45 ms per tick of `SetPosition` calls, and the following step gets slower (broadphase updates): L2-AS reduced-whole
  0.50 -> 0.96 ms per tick, articulated 5.1 -> 5.9 ms (unpinned). KSP pays 3.3 ms of script plus ~2.25 ms of extra PhysX for it
  (PROFILE.md 4).
- **Precision**: a vessel resting on the pad for 10 s (`precision`):

| build | physics-frame origin | S articulated: max speed / CoM drift | L2-AS reduced-whole: max speed / drift |
|---|---|---|---|
| double (`JPH_DOUBLE_PRECISION`) | Kerbin's centre (600 km away) | 6e-6 m/s / 1e-5 m | 0.12 m/s / 0.04 m |
| double | the pad | 8e-6 m/s / 1e-5 m | 0.12 m/s / 0.05 m |
| float | Kerbin's centre | 0.59 m/s / 0.18 m | 12.8 m/s / 12.9 m (unusable) |
| float | the pad | 5e-6 m/s / 8e-6 m | 0.10 m/s / 0.04 m |

  Double positions at the body centre behave like float positions at a local origin. (The L2-AS 0.1 m/s is contact jitter of a
  680 t rigid body standing on 49 engine bells; identical in both frames.)
- **Cost of double precision**: L2-AS, float build at the pad origin vs double build at the body centre: articulated 2.87 vs
  2.83 ms, reduced 0.328 vs 0.330 ms: no measurable difference (Jolt keeps doubles to positions only).
- **Orbit** (`results/orbit_frames.md`): PhysProf-M2-AS coasting one 80 km orbit (93,741 ticks), inertial frame:

| model | frame velocity (Galilean, re-centred at 50 m/s) | radius error max | joint stretch max |
|---|---|---|---|
| articulated | off (bodies at 2,279 m/s in float) | 193 m | 0.029 m |
| articulated | on (285 re-centres per orbit) | 23 m | 0.00006 m |
| reduced-whole | off | 24 m | -- |
| reduced-whole | on | 23 m | -- |

  Jolt's float velocities at orbital speed corrupt a jointed vessel (Jolt documents this, Docs/Architecture.md "Space
  Simulations"); a Galilean frame velocity (KSP's `Krakensbane.FrameVel` without the position shift) fixes it. The remaining
  23 m is the O(dt) error of semi-implicit Euler (240 m at 10x); KSP's orbit drift compensation (VesselPrecalculate.cs:303-343)
  exists for exactly this and is kept (WP-19a).
- **Determinism** (`results/determinism.md`): the same 30 s ascent, twice single-threaded and at 4 and 16 threads, gives
  bit-identical final states for all three models (Jolt + the kspcore ports in the loop).

### 2.7 Lever 2: the per-part pipeline natively (`results/pipeline_pinned.md`)

Per tick: part transforms (vessel-local float frame), `Vessel::UpdateMassStats` (WP-14), atmosphere (WP-09),
`DragCubeSetupAndPartAeroStats` + `UpdateAerodynamics` (WP-21, Mono float rule), engine forces:

| craft | parts | kernel | pipeline ms per tick | us per part | of which aero (UpdateAerodynamics) | aero setup | mass | outputs compared | mismatches |
|---|---|---|---|---|---|---|---|---|---|
| PhysProf-M2-AS | 153 | exact (WP-21 port as written) | 0.138 | 0.90 | 0.130 | 0.004 | 0.003 | -- | -- |
| PhysProf-M2-AS | 153 | SoA | 0.061 | 0.40 | 0.052 | 0.005 | 0.003 | 229,500 part-ticks | 0 |
| PhysProf-L2-AS | 457 | exact | 0.430 | 0.94 | 0.404 | 0.013 | 0.009 | -- | -- |
| PhysProf-L2-AS | 457 | SoA | 0.183 | 0.40 | 0.157 | 0.014 | 0.009 | 685,500 part-ticks | 0 |
| PhysProf-XL2-AS | 2,053 | exact | 1.697 | 0.83 | 1.573 | 0.067 | 0.043 | -- | -- |
| PhysProf-XL2-AS | 2,053 | SoA | 0.762 | 0.37 | 0.634 | 0.071 | 0.042 | 3,079,500 part-ticks | 0 |

(`results/pipeline_pinned.md`; the same pipeline inside the scaling runs measured 0.29-0.31 ms at 457 parts: +-30 % between
processes even pinned.)

- KSP spends 7.5 us per part per tick in FlightIntegrator alone (2.6 us of it `UpdateAerodynamics`, 2.2 us `UpdateMassStats`) on
  its machine; the native exact ports take 0.6-0.9 us per part for all of the above, 0.4 us with the SoA aero kernel.
- The SoA aero kernel evaluates the six Mach-only FloatCurves once per tick instead of up to 36 times per part and caches the
  constant per-face Cd curve; the float statements are the same, and the **outputs are bit-identical to the exact port on every
  part of every tick** (685,500 part-ticks for L2-AS, 3,079,500 for XL2-AS, 0 mismatches).

### 2.8 Lever 3: structural loads of the reduced model (`results/loads.md`)

The reduced model's per-joint loads against the articulated model's sub-joint impulses (sum of linear impulses / dt per
PartJoint, torque about the anchor centroid), same 8 s ascent, averaged over the last 50 ticks:

| craft | autostruts | reduced load solve | attach joints compared | force: median / p90 / max rel. diff | max load / capacity: articulated | reduced | solve ms per tick | factorisation ms (L blocks) |
|---|---|---|---|---|---|---|---|---|
| PhysProf-S | 0 | tree | 14 | 0.000 / 0.002 / 0.006 | 0.045 | 0.045 | 0.002 | -- |
| PhysProf-M2 | 0 | tree | 54 | 0.000 / 0.001 / 0.001 | 0.058 | 0.058 | 0.010 | -- |
| PhysProf-M2 | 0 | graph (direct) | 54 | 0.000 / 0.001 / 0.001 | 0.058 | 0.058 | 0.019 | 0.10 (146) |
| PhysProf-M2-AS | 152 | tree | 54 | 0.725 / 0.848 / 0.877 | 0.033 | 0.058 | 0.010 | -- |
| PhysProf-M2-AS | 152 | graph (direct) | 26 | 0.194 / 0.627 / 0.670 | 0.033 | 0.039 | 0.026 | 0.18 (293) |
| PhysProf-L2-AS | 456 | tree | 166 | 0.760 / 0.871 / 0.937 | 0.035 | 0.073 | 0.031 | -- |
| PhysProf-L2-AS | 456 | graph (direct) | 86 | 0.198 / 0.619 / 0.898 | 0.035 | 0.046 | 0.077 | 0.63 (901) |

- **Tree craft: the reduced model's loads are the articulated model's loads** (force median 0.0 %, max 0.6 %; the same max load /
  capacity).
- **Strutted craft**: the tree solve ignores the autostruts and over-predicts attach-joint loads (median +73-76 %, max load /
  capacity 0.058-0.073 vs 0.033-0.035): conservative. The indeterminate graph solve brings the median to ~20 % and the max ratio to
  0.039-0.046. The split between rigid parallel paths is not unique (in KSP it falls out of PhysX's iteration order), so a
  per-joint gate against the articulated model is not meaningful for strutted craft; scenario gates are (section 7).
- Cost: tree solve 0.01-0.03 ms per tick at 153-457 parts. Graph solve: PCG needs 170-300 iterations (1.2-6.3 ms per tick,
  too slow); the direct method (`structure_graph_direct.cpp`: sparse block Cholesky over 6x6 node blocks, greedy
  minimum-degree order, factored in the cluster frame where K is constant, so a tick is two block-triangular solves) gives the
  same loads at 0.02-0.08 ms per tick and 0.1-0.6 ms per refactorisation (structure or contact set change); the L2-AS factor
  has 901 off-diagonal blocks for 456 nodes (little fill: the autostrut graph is nearly a tree).

On the pad (`results/loads_pad.md`, contacts as point supports of the graph): L2-AS max load / capacity articulated 0.130, graph
0.157, tree 0.174.

**Capacity** (`results/capacity.md`): on the pad, engines off, gravity ramped up by 1 g per second until something breaks:

| craft | articulated: first break / at | reduced, tree solve | reduced, graph solve |
|---|---|---|---|
| PhysProf-S | radial decoupler / 33.7 g | radial decoupler / 33.7 g | radial decoupler / 33.7 g |
| PhysProf-M2 | Mainsail -> tank / 10.8 g | Mainsail -> tank / 29.0 g | Mainsail -> tank / 29.0 g |
| PhysProf-M2-AS | Mainsail -> tank / 18.8 g | Mainsail -> tank / 29.0 g | radial decoupler / 25.1 g |

The short S breaks at the same joint and the same load in all three. The 28 m M2 stack breaks earlier in the articulated
model, consistent with its soft angular springs letting the stack lean under its own weight: at 1 g the articulated Mainsail
joint already carries 2,263 kN m of bending torque that the rigid model does not have (force 2,075 vs 2,069 kN: equal). That
buckling-type failure comes from the joint compliance the decision drops; the reduced model is therefore stronger than the
articulated (KSP-like) model for tall unstrutted stacks under high static load (risk R2). KSP itself was not measured here.

### 2.9 Staging (`results/staging.md`)

Firing every decoupler 5 s into the ascent: articulated: remove the constraints (0.03 ms); reduced-whole: re-cluster 1 -> 49 bodies
and rebuild compound shapes (0.5 ms at 457 parts, one-off); reduced-split: remove the constraints (0.01 ms). All three models agree on
what happens next (the 420 t core with one Mainsail falls back, the boosters tumble and break up).

## 3. Recommendation for WP-20

**Build the reduced rigid-body model on Jolt Physics in kspcore, in a double-precision body-fixed frame with no per-tick
shift, with the structural graph as the failure model; keep the articulated model only as a test oracle.**

1. **Physics engine: Jolt Physics v5.6.0, inside kspcore** (not Chaos, not PhysX). Plain C++, MIT, deterministic per binary,
   `JPH_DOUBLE_PRECISION` for planet-scale positions, fast rigid-body / contact / query core. Vendor the release source when
   WP-20 starts (risk R9). Unreal never simulates vessels; it renders the core's double state camera-relative (the UE scale spike
   already showed rendering needs no rebasing, NEXT-STEPS.md session 2).
2. **Frame: double, body-fixed below the inverse-rotation threshold, inertial above (KSP's own switch), no floating-origin shift.**
   Krakensbane's API stays (`GetFrameVelocity()` is read at 39 call sites) as a Galilean frame velocity used only in the inertial
   regime, re-centred rarely with one velocity write per body. Terrain colliders are static in this frame. (Lever 1: exact.)
3. **Vessel model: reduced.** One Jolt body per rigid cluster. Clusters are split at articulation points that need to move:
   decouplers / separators are cut when they fire (the `reduced-whole` default; `reduced-split` keeps them as constraints, which
   costs more and is only needed where something must actually move), docking ports when docking or undocking, robotic parts
   (hinges, pistons, rotors as motorised constraints), wheels / legs (suspension), and anything the player marks flexible.
   Mass, CoM and inertia come from KSP's per-part rules; every module force keeps its point of application. (Lever 3:
   approximate; wobble and Kraken failure modes are dropped, as decided.)
4. **Failure model: the structural graph with KSP's capacity rule.** Every tick, the load through every attach joint follows
   from the cluster's motion and the per-part forces (tree solve, O(parts), validated equal to the articulated joint impulses on
   tree craft); where struts / autostruts / docking loops or several ground contacts exist, the indeterminate graph solve
   shares the load (direct sparse factorisation per structural change, two triangular solves per tick).
   A joint over capacity breaks and the cluster splits; decouplers fire through the same split.
5. **Per-part pipeline: native, on the exact ports, with SoA kernels where hot.** The ports already gated at 0 ULP run unchanged on
   part transforms written in a vessel-local float frame each tick; hot kernels get bit-identical SoA versions (aero first:
   2.5-2.6x, 0 mismatches). (Lever 2: exact.)
6. **Physics warp:** keep KSP's 1-4x as the default; the reduced model supports more (within 1 % of 1x up to 25x on the L2-AS
   ascent; substepping at 0.02 s gives ~60x real time at 457 parts on one core) if the user wants it later.

Why not the articulated model (one body per part, PhysX-equivalent joints) as the main model:

- it is the cost: the Jolt step is 2.5 ms (L2-AS, 960 sub-joints) to 13 ms (XL2-AS, 4,320) per tick single-threaded at the 50
  velocity steps it needs, against 0.004-0.005 ms for the reduced model; threads help only very large islands (XL2-AS 4.2x at
  16 threads, L2-AS none);
- it is fragile in exactly the way the user does not want to preserve: below ~30 velocity steps the PartJoint chain stretches by a
  metre and tears itself apart on the pad; at 50 steps the heaviest craft still shed boosters while sitting on the pad, and the
  unstrutted 88 m stack breaks during ascent (the profile's L2 collapsed in KSP too);
- it cannot do physics warp: -10 % altitude at 4x, broken at 8x;
- its one advantage, faithful wobble, is the behaviour the review classifies as droppable.

Its value is as a **reference**: the same craft extraction builds both, and the reduced model's loads are checked against the
articulated joint impulses (bench `loads`) in WP-20c's gate.

## 4. Exact / approximate / changed, per optimisation

"Exact" = the same numbers KSP computes for the quantities the gates check (up to summation order where stated); "approximate" =
same physics, different numbers within a stated bound; "changed" = a deliberate behaviour change (allowed by
PHYSICS-ENGINE-REVIEW.md section 10 and the user's decision on Kraken-style failure modes).

| # | optimisation (PROFILE.md lever) | class | what stays the same | what differs | measured |
|---|---|---|---|---|---|
| 1 | No per-tick world shift: double body-fixed / inertial frame, Jolt DOUBLE_PRECISION, `FrameVelocity` only as rare Galilean bookkeeping (lever 1) | exact | forces, gravity at the CoM, Coriolis / centrifugal, frame switch rule, `GetFrameVelocity()` semantics, orbits from the CoM state | float rounding of positions (better: double instead of float-near-origin) | KSP pays 3.3 ms script + ~2.25 ms PhysX per L2-AS ascent tick for the shift; re-adding a per-tick shift in Jolt costs +0.10-0.45 ms per tick of SetPosition calls plus a slower step (L2-AS reduced 0.50 -> 0.96 ms per tick); resting on the pad, body-centre origin == pad origin (precision bench); single-precision Jolt at the body centre: unusable (13 m drift in 10 s on the pad) |
| 2 | Native per-part pipeline on the exact ports, no per-part callbacks (lever 2) | exact | every statement of the WP-21 / WP-14 / WP-09 ports (Mono float rule), KSP's order | none (the ports are already gated 0 ULP) | 0.6-0.9 us/part per tick for transforms + mass + atmosphere + aero setup + aero + engines, vs FlightIntegrator 7.5 us/part in KSP |
| 2b | SoA aero kernel with Mach-only curves hoisted (lever 2) | exact (bit-identical) | the same float statements in the same order on the same inputs | evaluation count of the FloatCurves (per tick instead of per face) | 0 mismatches over 685,500 (L2-AS) / 3,079,500 (XL2-AS) part-ticks; aero 2.5-2.6x faster |
| 3 | Reduced rigid-body model: welded clusters + structural graph (lever 3) | approximate | mass, CoM and inertia (KSP's per-part rules summed), every external force at its point, trajectories, joint capacity rule (PartJoint.cs:307-342), breaking under overload, staging / decoupling | no joint compliance: wobble, bending, oscillation-driven failures and spawn collapse are gone (changed, allowed); strutted craft need the graph solve for load sharing | 30 s ascents within 0.01-0.4 % of the articulated model; tree-craft joint loads equal the articulated impulses (median 0.0 %, max 0.6 %); Jolt step 0.004-0.005 ms vs 2.5-13 ms (L2-AS, XL2-AS) |
| 3b | Physics warp beyond 4x on the reduced model | approximate | the physics | integration error of a larger dt (explicit forces, aero) | L2-AS powered ascent: within 1 % of 1x up to 25x (reduced-whole); the articulated model is -10 % at 4x and breaks at 8x |
| 7 | Same-vessel collision filtering by group id (lever 7) | exact | KSP's pairing rule (same vessel: ignore unless both `sameVesselCollision`) | O(1) per candidate pair instead of an O(C^2) `IgnoreCollision` rebuild per structural event | not timed (the rebuild was never in a KSP measurement window either) |
| -- | Articulated model kept for tests | approximate | PartJoint construction | Jolt's soft-constraint solver instead of PhysX PGS; needs >= 30 velocity steps; less stable than PhysX on the heaviest craft at rest (L2-AS boosters break off after ~25 s on the pad at 50 steps) | 2.5 ms (L2-AS, 50 steps) vs KSP PhysX 1.9-2.25 ms (no-shift ticks, other CPU) |
| -- | Spike simplifications that WP-20 must NOT keep | changed (spike only) | -- | collider proxies (drag-cube boxes), engine thrust along the part axis at full throttle (no WP-23 `ThrustUpdate`, no fuel drain), no thermal, no SAS, no buoyancy / CollisionEnhancer, contact shares approximated (equal share in the tree solve) | -- |

## 5. The interface WP-19 / WP-20 / WP-12 need from kspcore

Physics lives in kspcore (Jolt is plain C++, no engine dependency), so physics scenarios run headless on Windows and Linux CI,
and Unreal only renders what the core reports (vessel / part transforms in camera-relative doubles, PQS meshes). Chaos is not
used for vessels or terrain collision. The spike's `vessel_physics.hpp` is the prototype of the per-vessel interface.

### 5.1 Frames (WP-20 owns, WP-19 drives)

| concept | KSP 1.12.5 | kspcore |
|---|---|---|
| authoritative position | float `Transform.position` near a moving origin + `FloatingOrigin` offset (Assembly-CSharp/FloatingOrigin.cs:166-290) | double, physics frame = main body's body-fixed frame below the inverse-rotation threshold, the body's inertial frame above (Assembly-CSharp/OrbitPhysicsManager.cs:322 `checkReferenceFrame` decides, unchanged); Jolt `JPH_DOUBLE_PRECISION` bodies live in it directly |
| velocity frame | `Krakensbane.FrameVel`, engaged above 5 m/s, with a world shift every tick (Krakensbane.cs:121-147, FloatingOrigin.cs:146-150) | `FrameVelocity` (double) kept with KSP's API (`GetFrameVelocity()` has 39 call sites in 30 files: they port unchanged), but only as a Galilean bookkeeping value in the inertial regime (no static geometry there); it changes rarely (when a vessel's frame-relative speed passes a threshold: one velocity write per body), never moves positions. In the body-fixed regime it is zero |
| float maths of the ported modules | `partTransform.position` floats near the origin | each tick the core writes every `PartTransform` in a VESSEL-LOCAL float frame (origin = vessel CoM of the tick, rotation = physics-frame rotation); the exact ports (WP-21 aero, WP-22 thermal, WP-23 engines, WP-24 SAS / wheels, WP-25 docking) run unchanged on it; forces come back as (vector, local point) and are re-based with the double CoM |
| rebase | every tick while `FrameVel != 0` | on SOI change (re-express the bubble in the new body's frame), on the rotating <-> inertial switch (velocities +-omega x r, as KSP does), and when vessel switching moves the bubble by more than ~1,000 km: O(bodies in the bubble) |

### 5.2 `PhysicsWorld` (WP-20)

- `Step(dt)` inside WP-00's FixedUpdate group at the `PhysicsFixedUpdate` slot (after `Timing5`), never more than `PHYSICS_FRAME_DT_LIMIT` worth of ticks per frame.
- Layers: parts, terrain / KSC statics, (later) EVA kerbals, wheels' suspension queries. Same-vessel collision suppression is a
  `GroupFilter` on the vessel id (`sameVesselCollision` = sub-group flag): O(1), no rebuild on structural events (replaces
  CollisionManager.cs:80-168).
- Queries: `RayCast` / `LineCast` (CollisionEnhancer.cs:47-184 anti-tunnelling, ModuleEngines exhaust damage ModuleEngines.cs:1314,
  Vessel ground height Vessel.cs:2493-2712, FlightIntegrator sun raycast in scaled space), `OverlapSphere` (ModuleCargoBay.cs:374-531).
- Contacts: a listener that turns Jolt manifolds into per-part events `{part, other part or terrain, relative normal speed,
  point}` for `Part.HandleCollision` crash tolerance (Part.cs:3355-3415, 3613-3655), landed / splashed bookkeeping and the
  reduced model's support share.
- Static colliders: `AddStaticMesh(id, centre (double, physics frame), vertices (float, relative), triangles)`,
  `RemoveStaticMesh(id)`: the WP-12 quad interface; KSC / PQSCity meshes the same way.

### 5.3 `VesselPhysics` (WP-20, one per loaded unpacked vessel)

Built from the WP-13/14 part tree with KSP's rules (spike `craft_model.cpp`: Part.Start rigidbody / mass / inertia rules,
PartJoint.Create node choice, sub-joint count, stiffness and break capacity, autostrut anchors); the spike reproduces the
profile's runtime joint counts exactly.

- per tick: `ReadState()` (part poses / point velocities, vessel CoM, velocity, angular velocity, MOI: what
  VesselPrecalculate.CalculatePhysicsStats reads from rigidbodies today, VesselPrecalculate.cs:388-511), `AddForceAtPosition(part, F,
  p)` / `AddTorque(part, T)` (Part.AddForce* buffers, Part.cs:8663-8688), `AddUniformAcceleration(a)` (gravity + Coriolis +
  centrifugal, `integrationAccel`), `SetAngularDrag(part, d)`, `UpdateMass(part masses)` (FlightIntegrator.UpdateMassStats
  output), `Flush()` (FlightIntegrator.Integrate), `AfterStep()` (structural loads, breaking).
- structural events (WP-16 / WP-25 call them): `Decouple(joint)`, `Couple(vesselA, vesselB, nodes)` (docking),
  `Undock`, `BreakJoint`, `SetRigid`, `Pack()` / `Unpack()` (rails), `SetPartPhysicsless(part)`.
- results: per attach joint `{force, torque, capacity}`, break events, contact events.

### 5.4 WP-19 on top

- One registry (`FlightGlobals` replaces WP-14's `FlightRegistry` and WP-08/10's `FlightGlobals` lists, see STATUS.md WP-14
  "Left for WP-19").
- Load / unload and pack / unpack by `VesselRanges` (unchanged rules, Vessel.cs:3013-3034, OrbitPhysicsManager.cs:107-153):
  unpack creates the `VesselPhysics` from the part tree and the orbit state (Part.ResumeVelocity), pack destroys it (KSP keeps
  kinematic bodies; nothing in gameplay needs them once collisions are off).
- `VesselPrecalculate` ported against `VesselPhysics::ReadState()` (CoM, velocity, MOI) with the double frame: gravity at the
  CoM, Coriolis / centrifugal, easing, orbit drift compensation (VesselPrecalculate.cs:281-368) unchanged; `OrbitDriver.TrackRigidbody`
  from the double CoM and `FrameVelocity` (OrbitDriver.cs:221-249).

### 5.5 WP-12 on top

- The PQS quadtree and quad build in the core (`PQ`, `PQS.UpdateQuads` subdivision around the target vessel, vertex positions
  from WP-11's height stack, normals, UVs), positions in the body-fixed double frame (quad centre double + float vertices).
- `PQSMod_QuadMeshColliders` rule (subdivision >= maxLevel - |maxLevelOffset|, PQSMod_QuadMeshColliders.cs:17-44) decides which
  quads become `AddStaticMesh` colliders; the same mesh goes to Unreal for rendering through the C ABI.
- Because the physics frame is body-fixed below the threshold, terrain colliders never move; above the threshold no vessel is
  near terrain by construction (the threshold is above the highest terrain), so they are simply not needed there.

## 6. Risks

| # | risk | evidence / why | mitigation | owner |
|---|---|---|---|---|
| R1 | **Load sharing in strutted craft.** The tree solve is exact for trees but conservative where struts / autostruts / docking loops share load | bench `loads`: tree solve vs articulated joint impulses, median force +73-76 % on M2-AS / L2-AS (max load / capacity 0.058-0.073 vs 0.033-0.035) | the indeterminate graph solve prototyped in `structure_graph*.cpp` (every sub-joint and ground contact an elastic element; direct sparse factorisation per structural change, 0.02-0.08 ms per tick at 153-457 parts) brings the median difference to ~20 % and the max load / capacity to 0.039-0.046 vs 0.033-0.035. In KSP itself the split between rigid parallel paths is decided by PhysX's iteration order, so there is no exact reference: gate strutted craft by scenario (does it hold / break under the KSP capacity rule), not by per-joint numbers | WP-20c |
| R2 | **Players notice the missing flex.** Wobble, bending tall stacks, oscillation-driven failures disappear | PHYSICS-ENGINE-REVIEW.md 10 lists them as droppable; the user decided Kraken-style failure modes need not be preserved | keep "flexible" articulations available as Jolt constraints between clusters (`reduced-split` is that mechanism: decouplers today, robotics / docking ports / player-marked "flexible" joints tomorrow) with KSP's stiffness; the articulated model stays as a test oracle. Consequence to accept: tall unstrutted stacks are stronger than in the articulated (KSP-like) model under high static load (bench `capacity`: M2 first breaks at 29 g instead of 10.8 g; the short S breaks at the same 33.7 g in both) | WP-20b |
| R3 | **Jolt is a float solver** (velocities, rotations, broadphase AABBs) | Jolt Docs/Architecture.md "Big Worlds" / "Space Simulations"; the articulated model in orbit: M2-AS coasting at 2,279 m/s in float velocities: joint stretch 0.029 m and 193 m orbit error, against 0.00006 m and 23 m with a Galilean frame velocity (section 2.6) | double positions (`JPH_DOUBLE_PRECISION`; no measurable cost: L2-AS 2.83 ms double at the body centre vs 2.87 ms float at the pad, reduced 0.330 vs 0.328 ms); a Galilean `FrameVelocity` in the inertial regime keeps velocities small (one velocity write per body when re-centred, never a position shift); rebase the frame origin when a bubble is > ~1,000 km from the body centre (broadphase resolution 1 m at 10,000 km per Jolt's docs) | WP-20a / WP-19b |
| R4 | **Wheels and landing legs** use PhysX `WheelCollider` (suspension raycast + spring inside PhysX) plus Edy's VPP tyre solver (PHYSICS-ENGINE-REVIEW.md 7.1) | not prototyped | wheels as articulation points: suspension = ray / shape cast from the wheel part against the static and dynamic layers + spring-damper force on the cluster body at the contact (KSP's own split: PhysX did only raycast + spring); the VPP tyre maths is managed code (WP-29 ports it) | WP-29 + WP-20e |
| R5 | **Robotics (Breaking Ground)** need motorised hinges / pistons between parts | not prototyped | hinge / slider constraints with motors between clusters (Jolt `HingeConstraint` / `SliderConstraint` / `SixDOFConstraint` motors); a servo is an articulation point; same mechanism as R2 | WP-52 on WP-20b |
| R6 | **Contact fidelity with proxy colliders.** The spike collides drag-cube boxes, not the .mu colliders | the craft land on the pad and fly, but crash tolerance, landing stability and building impacts depend on real shapes | WP-20 needs the model reader's colliders (mesh / convex / box / sphere / capsule from the .mu files; CLAUDE.md "Original game assets"), as Jolt convex hulls / compound children; scenario gates `crash_tolerance`, `gear_settle` | WP-20b / UE import |
| R7 | **KSP quirks tied to rigidbodies** (per-part inertia floor, diagonal-only vessel MOI, angular drag as PhysX `angularDrag`, max angular velocity 50, landed FixedJoint anchor, wheel brake `FreezeAll`) | PHYSICS-ENGINE-REVIEW.md 9 | reproduce per part in the cluster sums (the spike does the floor and the angular drag); keep `CalculatePhysicsStats`' diagonal MOI for SAS tuning (WP-24 reads it); the landed anchor becomes "a landed, still cluster is set kinematic" | WP-19a / WP-20b |
| R8 | **Determinism across machines.** Scenario gates on two OSes need identical or tolerance-bounded results | Jolt is deterministic for the same binary and call order; measured: bit-identical final states run to run and at 1 / 4 / 16 threads for all three models (same binary); Windows vs Linux not measured | gate scenarios with tolerances, or build Jolt with `CROSS_PLATFORM_DETERMINISTIC` for the gate build (~8 % slower per Jolt's docs) and compare Windows / Linux bit for bit | WP-20a |
| R9 | **Dependency management.** The spike fetches Jolt over the network at configure time; CI and cloud VMs must build offline-safe | -- | vendor the Jolt release source (`third_party/jolt`, 6.1 MB for `Jolt/` + `Build/` + `LICENSE`, sha256 in `third_party/VERSIONS.md`, as doctest / nlohmann are) when WP-20 starts; keep the pinned version | WP-20a |
| R10 | **Physics warp semantics.** Physics warp beyond 4x changes gameplay (KSP caps it at 4x [runtime]) | bench `warp`: the reduced model stays within 1 % of the 1x ascent up to 25x | expose a configurable cap; default 4x for parity; offer higher warp as an option | WP-10 / WP-20 |
| R11 | **The per-part pipeline becomes the dominant cost** once the solver is gone | reduced model: pipeline ~89 % of the tick at 457 parts | SoA kernels (aero measured 2.5-2.6x, bit-identical), skip q = 0 aero (PHYSICS-ENGINE-REVIEW.md 11.10), per-vessel jobs for many vessels | WP-20d |
| R12 | **Multiple loaded vessels and debris** (staging, docking approach) were not profiled in KSP and only staging was measured here | bench `staging`: rebuild 0.5 ms one-off at 457 parts | one `VesselPhysics` per vessel in one world; per-vessel jobs; debris packs early (KSP ranges) | WP-19a |

## 7. Package split and gates

Implementer-sized packages (each 1-3 implementer sessions, ~2-6k lines), in dependency order. "Bit-exact" means 0 ULP against the
named reference, as every existing gate; "scenario" means a headless flight in the core with a stated tolerance and a
stated reference. The spike's code is the starting point of 20a-20d (move, do not copy-paste blindly: it is a spike).

| package | content (C# files) | depends on | gate: bit-exact | gate: scenario |
|---|---|---|---|---|
| **WP-20a physics world** | Jolt vendored (pinned v5.6.0, DOUBLE_PRECISION), layers, same-vessel `GroupFilter` (CollisionManager.cs, CollisionEnhancer.cs replaced by queries / CCD), body-fixed / inertial double frame + `FrameVelocity` (Krakensbane.cs, FloatingOrigin.cs: API kept, shift removed), `PhysicsWorld` (step in WP-00's FixedUpdate slot, queries, contact listener -> part events), static mesh API; PhysicsGlobals is done (WP-09) | WP-00, WP-09 | none (engine glue); unit tests: group filter = KSP's ignore rule on every pair of the PhysProf crafts, frame maths (rebase, rotating <-> inertial velocity change) against the C# formulas | `free_fall` (uniform gravity, 1 body, 10 s: analytic within 1e-9 relative); `determinism` (same scenario twice and at 1 / 4 / 16 threads: bit-identical); `frame_switch` (velocity / position continuous across the threshold and an SOI change within 1e-6 m) |
| **WP-20b vessel rigid bodies** | `VesselPhysics`: part -> cluster build with KSP's Part.Start / PartJoint / UpdateMassStats / ValidateInertiaTensor rules (spike `craft_model.cpp`, `reduced.cpp`), clusters split at articulation points (decouplers, docking ports, robotic parts, wheels, "flexible"), force buffers, mass updates, `ReadState` for VesselPrecalculate; the articulated model kept as a test-only reference (`articulated.cpp`); colliders from the model reader (proxy boxes until then) | WP-20a, WP-13/14/18 | craft extraction: sub-joint counts == PROFILE.md runtime counts (S 24, S-AS 44, M2 168, M2-AS 320, L2-AS 960), stiffness / break force per joint == the PartJoint formulas, rb.mass == `UpdateMassStats` port, cluster mass / CoM sums in a fixed order (double) | `ascent_rigid`: reduced vs articulated (50 steps) PhysProf-M2-AS / L2-AS 30 s ascent, altitude and speed within 0.5 % (measured 0.04 % / 0.26 %); `orbit_coast` drift (see WP-19a) |
| **WP-20c structural graph** | tree load solve + indeterminate graph solve (struts, autostruts, docking loops), capacity per PartJoint (PartJoint.cs:307-342), break -> split / decouple, `Couple` / `Undock` / `Decouple` used by WP-16 / WP-25 (Part.decouple, Part.Couple, Part.Undock), autostrut rebuild on vessel change (Part.cs:8023-8126) | WP-20b, WP-16, WP-25 | tree loads == articulated joint impulses on tree craft is a scenario (below); the capacity table per joint bit-exact vs the C# formula | `joint_loads` (bench `loads`): tree craft S / M2 force within 1 % of the articulated impulses (measured median 0.0 %, max 0.6 %); `joint_capacity` (bench `capacity`): a gravity ramp on the pad breaks the joint the capacity rule predicts at the predicted load (PhysProf-S: articulated and reduced both at 33.7 g), and a payload on a size-0 / 1 / 2 node breaks between 0.95x and 1.05x of the KSP capacity load and not below; `staging`: decouple all of L2-AS, part / mass conservation, one-off < 5 ms |
| **WP-20d flight-integrator tick** | FlightIntegrator.FixedUpdate sequence (UpdateMassStats, atmosphere, DragCubeSetupAndPartAeroStats, thermal, Integrate) over the ported members of WP-21 / WP-22, engine forces from WP-23 `ThrustUpdate`, torques from WP-24, in the measured order (PHYSICS-ENGINE-REVIEW.md 1.2); vessel-local float transforms; SoA kernels where hot (aero first) | WP-20b, WP-21, WP-22, WP-23, WP-24 | every SoA kernel == the AoS exact port, all outputs, every part, every tick of the PhysProf ascents (spike: 685,500 part-ticks, 0 mismatches); existing capture gates stay green | `ascent_profile` (TIERING.md): PhysProf-M2 30 s ascent within a few % of KSP's PROFILE.md window (2.9 km / 260 m/s for S, 6.1 km / 384 m/s for M2) once SAS (WP-24) is in the loop; better: a new in-game capture of CoM altitude / speed / q per tick for 30 s |
| **WP-20e contact, landing, water** | Part.OnCollisionEnter / HandleCollision crash tolerance (Part.cs:3355-3415, 3613-3655), landed / splashed from contacts (Vessel.checkLanded), PartBuoyancy as SoA (PartBuoyancy.cs), launch clamps as kinematic anchors, the landed anchor | WP-20b | crash-tolerance decision per contact == the C# rule on synthetic contacts | `crash_tolerance` (TIERING.md): a part hits the pad at 0.9x / 1.1x its tolerance: survives / explodes; `splashdown` buoyancy equilibrium depth within 2 % of the analytic one |
| **WP-19a flight registry, rails, precalc** | FlightGlobals.cs (one registry), VesselRanges load / unload / pack / unpack (Vessel.cs:3013-3034, OrbitPhysicsManager.cs:107-153), ProtoVessel / ProtoPartSnapshot live paths (WP-04 / WP-14 data side exists), VesselPrecalculate.cs on `VesselPhysics` (gravity, Coriolis, centrifugal, easing, drift compensation), OrbitDriver TRACK_Phys from the double state, QuickSaveLoad, VesselSwitching | WP-20b, WP-10 | precalc formulas (gravity, Coriolis, centrifugal, drift-compensation offsets) bit-exact vs the C# on recorded states (an in-game capture of VesselPrecalculate in/out per tick would make it a capture gate) | `save_load_cycle` (TIERING.md): quicksave / quickload on rails and off rails, positions within 1e-6 m (rails) and 1e-3 m (physics); `physics_orbit_hold`: a coasting vessel with drift compensation stays on its Kepler orbit within 1 m over 10 orbits (spike without compensation: 23-24 m radius error over one 80 km orbit at dt 0.02, O(dt)); rails <-> physics transitions preserve state vectors within 1e-6 m (WP-10's acceptance) |
| **WP-19b frames and bubble** | OrbitPhysicsManager.checkReferenceFrame (rotating <-> inertial), SOI change of the bubble, `FrameVelocity` re-centring, rebase on vessel switch, multiple loaded vessels | WP-19a | -- | `frame_switch` (WP-20a), `two_vessels` (a second vessel loads at 2.25 km, unpacks at 200 m, both coast; relative position error < 1 mm) |
| **WP-12a PQS quadtree and quad build** | PQ.cs, PQS.cs quad subdivision around the target (PQS.UpdateQuads, PQ thresholds PQ.cs:280-346), PQSMeshPlanet, PQSCache, vertex build (positions from WP-11 heights, normals, UVs) in the body-fixed double frame; PQSLandControl / colour mods later | WP-11, WP-00 | quad vertex heights == WP-11 `GetSurfaceHeight` at the vertex directions (bit-exact, they are the same call); vertex positions double | `quad_subdivision`: quad count / levels around KSC from a fixed camera / target vs an in-game capture of `PQS.quads` (needs a capture); render handoff checked by the UE automation test |
| **WP-12b terrain colliders** | PQSMod_QuadMeshColliders rule, PQS_Collider*, PQS_PartCollider, PQSCity / PQSCity2 / KSC meshes as static colliders through `AddStaticMesh` | WP-12a, WP-20a, WP-42 | collider triangles == the quad mesh (same buffers) | `ksc_ground` (TIERING.md): ray heights on the pad / runway / around KSC vs `capture.terrain_height` within the triangulation error (< 0.05 m on the flattened pad); a craft spawned on the pad settles within 0.1 m of KSP's spawn height |

Order: 20a -> 20b -> (20c || 20d || 19a) -> 20e -> 19b; 12a can start now (needs only WP-11 / WP-00), 12b after 20a.

TIERING.md's WP-20 acceptance lists `joint_stiffness` (vs KSP traces): with the reduced model there is no joint compliance to
compare, so it becomes `joint_loads` + `joint_capacity` above; `free_fall`, `physics_orbit_hold` and `crash_tolerance` stay.
Captures (profiling branch) that would turn scenarios into capture gates: (1) a 30 s PhysProf-M2 ascent with SAS on, CoM
altitude / speed / q / Mach per tick (`ascent_profile`); (2) `VesselPrecalculate` inputs and outputs per tick on the pad, in
ascent and in orbit (`integrationAccel`, Coriolis, centrifugal, drift-compensation offsets) for WP-19a; (3) the PQS quad set
around KSC from a fixed target (`quad_subdivision`); (4) KSP's own first-break load under a slow gravity / thrust ramp for
PhysProf-S and -M2 (the `capacity` bench run in the game) to settle how much of KSP's early M2 failure is compliance.
Not in these packages: wheels and legs (WP-29 on 20b / 20e), robotics (WP-52 on 20b), EVA kerbal rigidbody (WP-32 on 20b),
rendering of vessels and terrain (UE shell).

## 8. Reproducing

```powershell
# from core/ (worktree or main checkout); CMAKE_BUILD_PARALLEL_LEVEL is honoured
powershell -NoProfile -ExecutionPolicy Bypass -File spikes\jolt\build_spike.ps1                         # build/spike-jolt (double)
powershell -NoProfile -ExecutionPolicy Bypass -File spikes\jolt\build_spike.ps1 -Preset spike-jolt-f32  # build/spike-jolt-f32
build\spike-jolt\spikes\jolt\jolt_spike.exe info PhysProf-S PhysProf-L2-AS                             # craft model, joint counts
build\spike-jolt\spikes\jolt\jolt_spike.exe flight PhysProf-L2-AS --model reduced-whole --samples      # one scenario
bash spikes/jolt/tools/run_benches.sh build/spike-jolt                                                # every set -> results/
build\spike-jolt\spikes\jolt\jolt_spike.exe bench scaling --affinity 4 --repeat 3                      # the pinned table
```

Options of `flight` / `bench` (see `src/bench.cpp` `ConfigFromArgs`): `--model articulated|reduced-whole|reduced-split`,
`--vel-steps / --pos-steps`, `--threads`, `--warp <rate>`, `--pad` (engines off), `--no-aero`, `--kernel exact|soa|both`,
`--origin-pad`, `--shift`, `--orbit [--orbit-alt m] [--frame-velocity]`, `--graph [--graph-pcg] [--graph-klin k]`,
`--stage-at <tick>`, `--no-break`, `--debug`, `--affinity <hex mask>`, `--repeat <n>`, `--csv <file>`.
Bench sets: `scaling`, `iterations`, `threads`, `warp`, `shift`, `precision`, `pipeline`, `staging`, `orbit` (`--frames off,on`),
`loads`, `capacity`, `determinism`, `selftest`.

The configure step downloads Jolt (19 MB) once per build tree and generates the XL crafts with the profile's generator
(`analysis/physics/scripts/gen_craft.py` through `tools/gen_xl_crafts.py`) into the build tree. Nothing reads
`decompiled/KSP-dev`: game data comes from `core/testdata/GameData-mirror`, terrain from `core/data/terrain` (both committed).

