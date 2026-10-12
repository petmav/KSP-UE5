# WP-20c: the production structural loads against the spike's articulated model, and the cost of the load solve (STATUS.md "WP-20c")

Machine: Ryzen 9 7950X3D, Windows 11, MSVC 14.44, Release (the `spike-jolt` tree). Commands (from `core/`, after `spikes\jolt\build_spike.ps1`):

    build\spike-jolt\spikes\jolt\wp20c_loads.exe
    build\spike-jolt\spikes\jolt\wp20b_bench.exe ascent PhysProf-S PhysProf-M2-AS PhysProf-L2-AS build\spike-jolt\spikes\jolt\craft\PhysProf-XL1-AS.craft build\spike-jolt\spikes\jolt\craft\PhysProf-XL2-AS.craft --repeat 3 --affinity 4 --loads none|tree|auto

## Loads against the articulated (PhysX-equivalent) model (PHYSICS-SPIKE.md 7, `joint_loads`)

`wp20c_loads` runs the spike's articulated model (one Jolt body per part, every PartJoint sub-joint a SixDOF constraint with KSP's drives, 50
velocity steps; loads = the sub-joint impulses / dt, summed per PartJoint, torque about the anchor centroid) and the PRODUCTION binding
(`VesselPhysics::JointLoads`, `LoadSolve::Auto`) on the spike's ascent (LaunchPad on 30 x 30 PQS quads, 2 s settle, full thrust, WP-21 exact aero)
for 8 s; per attach joint the force and torque magnitudes averaged over the last 50 ticks; joints carrying >= 5 % of the largest load compared
(the spike's `loads` bench, whose numbers are in brackets).

| craft | autostruts | production solve | joints compared | force: median / p90 / max rel. diff | torque: median / p90 | max sub-joint load / capacity (production) | load solve ms per tick (unpinned) |
|---|---|---|---|---|---|---|---|
| PhysProf-S | 0 | tree | 14 | 0.0002 / 0.0021 / 0.0053 (spike tree 0.000 / 0.002 / 0.006) | 0.027 / 1.000 | 0.0448 (spike 0.045) | 0.0026 |
| PhysProf-M2 | 0 | tree | 54 | 0.0002 / 0.0005 / 0.0007 (0.000 / 0.001 / 0.001) | 0.007 / 0.992 | 0.0581 (0.058) | 0.0177 |
| PhysProf-M2-AS | 152 | graph | 26 | 0.1940 / 0.6271 / 0.6700 (spike graph 0.194 / 0.627 / 0.670) | 0.996 / 0.999 | 0.0389 (0.039) | 0.0407 |
| PhysProf-L2-AS | 456 | graph | 86 | 0.1987 / 0.6186 / 0.8979 (0.198 / 0.619 / 0.898) | 0.993 / 1.000 | 0.0458 (0.046) | 0.1340 |

- Tree craft: the production loads ARE the articulated impulses (the gate's 1 %: max 0.53 % on S, 0.07 % on M2).
- Strutted craft: the elastic split of the graph solve is not PhysX's (median ~20 %); there is no exact reference (PHYSICS-SPIKE.md R1: PhysX's
  split between rigid parallel paths follows its iteration order). The production numbers equal the spike's graph solve.
- Torques: the articulated model's angular drives carry bending the rigid ring does not; the few torque-loaded joints differ (p90 ~1).

## Tick cost with the load solve (ms per 0.02 s tick, one thread, pinned (`--affinity 4`), best of 3)

| craft | parts | no loads | tree solve: tick (solve) | Auto: tick (solve) | Auto: factorisation (last) |
|---|---|---|---|---|---|
| PhysProf-S | 21 | 0.0179 | 0.0201 (0.0018) | 0.0201 (tree, 0.0019) | -- |
| PhysProf-M2-AS | 153 | 0.1155 | 0.1297 (0.0130) | 0.1481 (graph, 0.0307) | 0.24 ms |
| PhysProf-L2-AS | 457 | 0.3478 | 0.3913 (0.0393) | 0.4440 (graph, 0.0931) | 1.14 ms |
| PhysProf-XL1-AS | 1,027 | 0.8130 | 0.9028 (0.0896) | 1.0603 (graph, 0.2175) | 2.21 ms |
| PhysProf-XL2-AS | 2,053 | 1.7186 | 1.9299 (0.1935) | 2.1502 (graph, 0.4385) | 6.39 ms |

A second session (best of 5, a busier machine) measured the same within +5..15 % (L2-AS 0.371 / 0.431 / 0.473 ms; the factorisation times
come from it). Max load / capacity over the 28 s ascent (production, Auto): S 0.113 (radial decoupler), M2-AS 0.044, L2-AS 0.046, XL1-AS 0.050,
XL2-AS 0.051; three factorisations per run (the autostruts secured after the first step, touchdown on the pad, liftoff).
