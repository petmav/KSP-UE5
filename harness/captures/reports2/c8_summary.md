# C8 orbit_rails_ingame summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:08:17.0056415Z`

| seg | requested | achieved | frames | UT start | UT end |
|---|---|---|---|---|---|
| 0 | 1 | 1 | 500 | 2.30091e+06 | 2.30092e+06 |
| 1 | 100 | 100 | 1000 | 2.30092e+06 | 2.30292e+06 |
| 2 | 1000 | 1000 | 1000 | 2.30294e+06 | 2.32294e+06 |
| 3 | 10000 | 10000 | 1500 | 2.32314e+06 | 2.62314e+06 |
| 4 | 100000 | 100000 | 500 | 2.62514e+06 | 3.62514e+06 |

| id | body | inc | e | sma | epoch | note |
|---|---|---|---|---|---|---|
| kerbin_leo_circ_equ_a | Kerbin | 0 | 0 | 680000 | 2.30091e+06 | harness case |
| kerbin_gto_a | Kerbin | 5 | 0.5 | 6e+06 | 2.30091e+06 | harness case |
| kerbin_e081_sma4000km | Kerbin | 3 | 0.81 | 4e+06 | 2.30091e+06 | harness kerbin_e081_a with sma 4,000 km (3,000 km has periapsis below the surface) |
| kerbin_hyp12_a | Kerbin | 10 | 1.2 | -4e+06 | 2.30091e+06 | harness case; escapes Kerbin SOI to the Sun |
| mun_incl_ell_a | Mun | 45 | 0.3 | 500000 | 2.30091e+06 | harness case |
| mun_encounter | Kerbin | 0 | 0.888559 | 7.53154e+06 | 2.31089e+06 | Mun encounter: hyperbola sign 1, tPeri=ut0+15000, predicted SOI entry ut0+9986.9970816746354 |

## Consistency

pos = the Orbit's own state after OrbitDriver.UpdateOrbit this frame; relpos = orbit.getRelativePositionAtUT(UT) recomputed; world = GetWorldPos3D() - mainBody.position (Unity axes, compared with pos.xzy; includes the rotated localCoM offset). kepler = independent double-precision two-body propagation from the spawned elements (only for samples on the initial patch), mapped through the captured Planetarium.Zup frame (relpos = Zup.WorldToLocal(inertial)).

| id | samples | max |pos-relpos|/r | max |world-pos.xzy|/r | max |kepler-relpos|/r | max |kepler-relpos| m |
|---|---|---|---|---|---|
| body:Minmus | 450 | 8.08858e-16 | 7.92615e-17 | 0 | 0 |
| body:Mun | 450 | 0 | 7.76102e-17 | 0 | 0 |
| kerbin_e081_sma4000km | 450 | 4.60427e-16 | 1.53177e-16 | 9.27103e-14 | 1.21473e-07 |
| kerbin_gto_a | 450 | 3.92773e-16 | 1.15459e-16 | 2.27046e-14 | 1.11863e-07 |
| kerbin_hyp12_a | 450 | 6.84659e-16 | 1.43386e-16 | 1.32841e-14 | 6.92198e-07 |
| kerbin_leo_circ_equ_a | 450 | 0 | 1.91406e-16 | 4.73507e-13 | 3.21985e-07 |
| mun_encounter | 315 | 8.29375e-16 | 3.32554e-15 | 1.06817e-15 | 9.72329e-09 |
| mun_incl_ell_a | 450 | 3.2867e-16 | 2.95825e-15 | 2.41844e-13 | 8.47249e-08 |

## SOI transitions

| vessel | UT | from | to | new sma | new e | warp |
|---|---|---|---|---|---|---|
| HC8 mun_encounter | 2.3109e+06 | Kerbin | Mun | -497073 | 1.50294 | 1000 |
| HC8 mun_encounter | 2.32094e+06 | Mun | Kerbin | 7.53154e+06 | 0.888559 | 1000 |
| HC8 kerbin_hyp12_a | 2.37934e+06 | Kerbin | Sun | 1.22962e+10 | 0.140348 | 10000 |
| HC8 mun_encounter | 2.44894e+06 | Kerbin | Mun | -460160 | 1.0171 | 10000 |
| Ast. QNQ-893 | 3.13314e+06 | Sun | Kerbin | 4.66831e+07 | 0.873616 | 100000 |
| Ast. XHV-408 | 3.47914e+06 | Sun | Kerbin | -8.22501e+08 | 1.01477 | 100000 |
