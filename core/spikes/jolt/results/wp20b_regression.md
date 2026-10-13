# WP-20b: the production vessel binding against the spike's reduced model (STATUS.md "WP-20b")

Machine: Ryzen 9 7950X3D, Windows 11, MSVC 14.44, Release (the `spike-jolt` tree), pinned to logical CPU 2 (`--affinity 4`) at high
priority, best of 3 (`--repeat 3`). Scenario: the spike's `scaling` ascent (PhysProf crafts on the LaunchPad of Kerbin's real PQS terrain,
30 x 30 static mesh quads; 2 s settle, then 28 s at full throttle, WP-21 exact aero, thrust along the engine axis); "tick" is the mean
over the 28 s. Commands (from `core/`, after `spikes\jolt\build_spike.ps1`):

    build\spike-jolt\spikes\jolt\jolt_spike.exe bench scaling --models reduced-whole --affinity 4 --repeat 3
    build\spike-jolt\spikes\jolt\wp20b_bench.exe ascent PhysProf-S PhysProf-M2-AS PhysProf-L2-AS build\spike-jolt\spikes\jolt\craft\PhysProf-XL1-AS.craft build\spike-jolt\spikes\jolt\craft\PhysProf-XL2-AS.craft --spike-damping --repeat 3 --affinity 4
    build\spike-jolt\spikes\jolt\wp20b_bench.exe ascent <same crafts> --repeat 3 --affinity 4

## Tick time (ms per 0.02 s tick, one thread)

| craft | parts | spike reduced-whole (same session) | WP-20b, spike damping | WP-20b, per-part damping (default) | of which: pipeline / ReadState + gravity / Flush / step / AfterStep (WP-20b default) |
|---|---|---|---|---|---|
| PhysProf-S | 21 | 0.017 | 0.0173 | 0.0175 | 0.0135 / 0.0013 / 0.0001 / 0.0020 / 0.0005 |
| PhysProf-M2-AS | 153 | 0.105 | 0.1122 | 0.1127 | 0.0975 / 0.0093 / 0.0007 / 0.0021 / 0.0031 |
| PhysProf-L2-AS | 457 | 0.312 | 0.3371 | 0.3401 | 0.2979 / 0.0279 / 0.0020 / 0.0028 / 0.0095 |
| PhysProf-XL1-AS | 1,027 | 0.854 | 0.7878 | 0.7934 | 0.6971 / 0.0657 / 0.0050 / 0.0034 / 0.0222 |
| PhysProf-XL2-AS | 2,053 | 1.484 | 1.6190 | 1.6174 | 1.4159 / 0.1407 / 0.0101 / 0.0038 / 0.0468 |

The spike's tick contains its tree load solve (`after`: 0.027 ms at 457 parts; WP-20c's here) and no CalculatePhysicsStats MOI;
WP-20b's ReadState is KSP's full CalculatePhysicsStats (VesselPrecalculate.cs:388) over the parts, MOI loop included (0.021 ms at 457
parts, bit-identical to the WP-14 port). The per-part pipeline (the WP-21 exact aero port over the parts) is the same code in both.

## Trajectories after 30 s (altitude m / speed m/s of the vessel CoM)

| craft | spike reduced-whole | WP-20b, spike damping | spike articulated | WP-20b, per-part damping |
|---|---|---|---|---|
| PhysProf-S (tumbles at Mach 1 in every model) | 1170.6 / 69.7 | 1171.7 / 69.9 | 6477.1 / 217.6 | 5487.0 / 231.9 |
| PhysProf-M2-AS | 4921.8 / 333.0 | 4917.3 / 332.2 (-0.09 % / -0.23 %) | 4909.2 / 330.2 | 4917.8 / 331.9 (+0.17 % / +0.51 %) |
| PhysProf-L2-AS | 3043.6 / 204.9 | 3043.9 / 204.9 (+0.01 % / +0.01 %) | 3044.9 / 206.0 | 3045.4 / 205.4 (+0.02 % / -0.30 %) |
| PhysProf-XL1-AS | 2589.0 / 169.8 | 2589.1 / 169.8 (+0.005 % / +0.02 %) | 2588.7 / 170.5 | 2589.3 / 169.9 |
| PhysProf-XL2-AS | 2487.1 / 155.9 | 2487.1 / 155.9 (+0.002 % / +0.001 %) | 2496.3 / 156.2 | 2487.1 / 155.9 |

PhysProf-S after 8 s (`--ticks 400`, before the tumble): spike reduced-whole 870.172 / 262.358, WP-20b spike damping 870.172 / 262.357;
spike articulated 870.093 / 262.273, WP-20b per-part damping 870.168 / 262.349.

## Pad, 10 s at rest (5 s settle, then 10 s; engines off, no aero)

| craft | spike reduced-whole: max speed / CoM drift | WP-20b |
|---|---|---|
| PhysProf-S | 0 / 0 | 3.3e-9 m/s / 3.8e-10 m |
| PhysProf-M2-AS | 0 / 0 | 1.3e-8 m/s / 1.3e-9 m |
| PhysProf-L2-AS | 0.120 m/s / 0.043 m | 0.120 m/s / 0.036 m |

On a 4 x 4 quad patch instead of the spike's 30 x 30 the M2-AS keeps rocking at 0.07 m/s (9 mm): Jolt orders contact constraints by
body id, and on a statically indeterminate support (9-49 engine bells) a different order can leave the iterative contact solver in a
small limit cycle; the fixture therefore builds the spike's patch.
