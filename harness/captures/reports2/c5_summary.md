# C5 deltav_readout summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:06:13.7312159Z`

| craft | parts | stages | mass t | Kerbin vac | Kerbin ASL | Mun ASL | Duna ASL | Eve ASL | note |
|---|---|---|---|---|---|---|---|---|---|
| PhysProf-S | 21 | 2 | 35.2 | 3011.24 | 2638.2 | 3011.24 | 2986.4 | 1128.87 |  |
| PhysProf-M2 | 153 | 2 | 218.1 | 6326.71 | 5701.6 | 6326.71 | 6285.32 | 2745.85 |  |
| PhysProf-L2-AS | 457 | 2 | 677.3 | 7041.19 | 6367.69 | 7041.19 | 6996.64 | 3108.27 |  |
| craft_Kerbal_X | 73 | 8 | 128.955 | 6662.43 | 4384.31 | 6662.43 | 6507.52 | 1841.59 |  |
| craft_Jumping_Flea | 8 | 2 | 2.57 | 849.818 | 721.057 | 849.818 | 841.266 | 145.452 |  |
| craft_Kerbal_1 | 33 | 6 | 18.7275 | 4213.02 | 2122.87 | 4213.02 | 4070.53 | 393.882 |  |
| craft_Kerbal_2 | 17 | 3 | 15.574 | 923.123 | 848.678 | 923.123 | 918.226 | 440.16 |  |
| craft_Learstar_A1 | 87 | 5 | 322.17 | 4618.08 | 3721.69 | 4618.62 | 4598.02 | 1056.76 |  |
| craft_Ion-Powered_Space_Probe | 10 | 1 | 0.726 | 4301.78 | 102.424 | 4301.78 | 4014.18 | 0.00102423 |  |
| craft_Dynawing | 126 | 5 | 235.53 | 3533.81 | 3101.47 | 3533.81 | 3504.89 | 1666.27 |  |
| craft_ComSat_Lx | 18 | 4 | 8.05139 | 5418.32 | 3015.98 | 5418.32 | 5254.8 | 470.418 |  |
| craft_Aeris_3A | 18 | 1 | 5.555 | 0 | 25060.2 | 25060.2 | 25060.2 | 25060.2 |  |
| craft_Aeris_4A | 40 | 2 | 19.36 | 0 | 11719.4 | 11719.4 | 11719.4 | 11719.4 |  |
| craft_Gull | 44 | 1 | 3.7645 | 0 | 8944.2 | 8944.2 | 8944.2 | 8944.2 |  |
| craft_Stearwing_A300 | 56 | 1 | 49.039 | 0 | 25054 | 25054 | 25054 | 25054 |  |
| craft_Bug-E_Buggy | 26 | 1 | 1.287 | 0 | 0 | 0 | 0 | 0 | IsReady=false after 15 s (no engines?) |
| craft_Mallard | 38 | 1 | 37.015 | 0 | 38310.7 | 38310.7 | 38310.7 | 38310.7 |  |

Rocket-equation cross-check (Kerbin rows, stages with ispVac > 0 and startMass > endMass): 20 of 24 stages agree with Isp g0 ln(m0/m1) (g0 = 9.80665) to max rel 6.6e-07 (float32 storage); the 4 others are multi-phase stages (engines with different Isp or burn times; the stage simulation integrates over burn phases and reports a combined Isp):

| craft | stage | ispVac | startMass | endMass | deltaVinVac | rel diff | engines active |
|---|---|---|---|---|---|---|---|
| PhysProf-S | 1 | 312.173 | 35.2 | 13.2 | 3011.24 | 0.00284146 | 3 |
| craft_Learstar_A1 | 4 | 335.87 | 321.87 | 141.87 | 2697.89 | 0.000176951 | 3 |
| craft_Learstar_A1 | 0 | 250 | 14.278 | 12.6402 | 297.43 | 0.00425878 | 5 |
| craft_Dynawing | 0 | 250 | 44.49 | 38.49 | 355.161 | 2.71166e-06 | 4 |

Note: in the editor, deltaVatASL and deltaVActual use DeltaVAppValues.body's pressure at DeltaVAppValues.altitude, so 'ASL' follows the selected body.
