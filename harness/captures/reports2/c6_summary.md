# C6 thermal_step summary

- ksp_version: `1.12.5 build 03190`
- unity_version: `2019.4.18f1`
- runtime: `Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)`
- assembly_csharp_sha256: `42d4b00db940c91eecf5d75ad1d95a4632c80f6322f2aad753f246b9335624cd`
- assembly_csharp_jit: `IsJITOptimizerDisabled=False`
- profiling_branch_commit: `36ef51bd9d095f57904e29e7b98886d0fd753d1d`
- utc: `2026-10-09T00:09:31.6539729Z`

| case | frames | UT0 | UT1 | alt0 | alt1 | max mach | max T_ext | max conv coeff | analytic frames |
|---|---|---|---|---|---|---|---|---|---|
| pad | 500 | 11.7 | 21.68 | 79.2884 | 79.2884 | 2.65666e-06 | 312.937 | 36.3146 | 0 |
| orbit | 1000 | 3.62517e+06 | 3.62519e+06 | 100001 | 100001 | 0 | 4 | 0 | 0 |
| reentry | 1500 | 3.62519e+06 | 3.62522e+06 | 78938.9 | 66106.5 | 8.10215 | 7109.58 | 25.172 | 0 |

| case | part | T0 | T1 | skin0 | skin max | skin1 |
|---|---|---|---|---|---|---|
| pad | probeStackLarge#294000007 | 309.143 | 309.175 | 309.688 | 309.766 | 309.766 |
| pad | Rockomax32.BW#294000014 | 309.126 | 309.126 | 309.317 | 309.344 | 309.344 |
| pad | liquidEngineMainsail.v2#294000021 | 309.126 | 309.127 | 309.171 | 309.171 | 309.171 |
| pad | liquidEngine2#294000042 | 309.127 | 309.128 | 309.207 | 309.215 | 309.215 |
| pad | pointyNoseConeB#294000084 | 309.132 | 309.149 | 309.483 | 309.731 | 309.731 |
| orbit | probeStackLarge#294000007 | 299.576 | 300.102 | 300.397 | 302.01 | 302.01 |
| orbit | Rockomax32.BW#294000014 | 299.536 | 300.254 | 302.078 | 303.584 | 303.584 |
| orbit | liquidEngineMainsail.v2#294000021 | 322.024 | 319.498 | 316.184 | 316.519 | 315.154 |
| orbit | liquidEngine2#294000042 | 330.417 | 327.42 | 323.122 | 323.142 | 321.58 |
| orbit | pointyNoseConeB#294000084 | 299.43 | 299.419 | 304.333 | 304.333 | 299.626 |
| reentry | probeStackLarge#294000007 | 300.18 | 301.717 | 302.036 | 427.843 | 427.843 |
| reentry | Rockomax32.BW#294000014 | 300.332 | 301.403 | 303.574 | 427.52 | 427.52 |
| reentry | liquidEngineMainsail.v2#294000021 | 319.223 | 317.915 | 314.942 | 489.742 | 489.742 |
| reentry | liquidEngine2#294000042 | 327.105 | 323.828 | 321.318 | 388.993 | 388.993 |
| reentry | pointyNoseConeB#294000084 | 299.405 | 299.261 | 299.366 | 332.247 | 332.247 |
