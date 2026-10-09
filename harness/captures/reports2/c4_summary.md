# C4 atmosphere_thermal_offsets summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:06:41.1826659Z`

| epoch | UT | offset s |
|---|---|---|
| 0 | 21.68 | 0 |
| 1 | 5421.74 | 5400 |
| 2 | 10821.7 | 10800 |
| 3 | 2.30091e+06 | 2.30089e+06 |

| body | epoch | min offset K | max offset K | min T(alt,offset) K | max T K | rows |
|---|---|---|---|---|---|---|
| Duna | 0 | -47.5 | 37.9999 | 150 | 271 | 112 |
| Duna | 1 | -47.5 | 36.8508 | 150 | 269.851 | 112 |
| Duna | 2 | -47.5 | 36.7017 | 150 | 269.702 | 112 |
| Duna | 3 | -47.5018 | 37.9975 | 150 | 270.998 | 112 |
| Eve | 0 | -56.0005 | 8.38969 | 90.8135 | 428.39 | 112 |
| Eve | 1 | -56.001 | 8.14048 | 90.8128 | 428.14 | 112 |
| Eve | 2 | -56.0018 | 8.89857 | 90.8112 | 428.899 | 112 |
| Eve | 3 | -56.083 | 8.98852 | 90.6591 | 428.989 | 112 |
| Jool | 0 | -37.5065 | 38.8826 | 120 | 225.54 | 112 |
| Jool | 1 | -37.5065 | 37.9032 | 120 | 224.187 | 112 |
| Jool | 2 | -37.5066 | 38.9839 | 120 | 225.681 | 112 |
| Jool | 3 | -37.515 | 38.0936 | 119.988 | 224.45 | 112 |
| Kerbin | 0 | -47.5001 | 24.707 | 158.805 | 312.857 | 112 |
| Kerbin | 1 | -47.5001 | 24.7071 | 158.805 | 312.857 | 112 |
| Kerbin | 2 | -47.5001 | 24.7071 | 158.805 | 312.857 | 112 |
| Kerbin | 3 | -47.5001 | 25.12 | 158.805 | 313.27 | 112 |
| Laythe | 0 | -29.0025 | 11.0409 | 175.948 | 288.041 | 112 |
| Laythe | 1 | -29.0025 | 10.8325 | 175.948 | 287.832 | 112 |
| Laythe | 2 | -29.0025 | 10.648 | 175.948 | 287.648 | 112 |
| Laythe | 3 | -29.0059 | 10.2156 | 175.944 | 287.216 | 112 |
| Mun | 0 | 0 | 0 | 0 | 0 | 112 |
| Mun | 1 | 0 | 0 | 0 | 0 | 112 |
| Mun | 2 | 0 | 0 | 0 | 0 | 112 |
| Mun | 3 | 0 | 0 | 0 | 0 | 112 |

GetFullTemperature(Vector3d pos) returns the *offset* (not a temperature) computed with an unnormalised sun vector; it differs from the FlightIntegrator-path offset in 2240 of 2688 rows.
