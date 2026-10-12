# cost: per-tick phase times (ms), mean over 20 s of game time, best of 3 runs; rovers at full throttle, the lander at rest. managed = the ported C# of every wheel (8 VPP substeps: active vessel); native = rays + PhysX-law forces + limit PGS (explicit) or VehicleConstraint parameter update + readback (jolt-vc); the jolt-vc rays and spring rows run inside the Jolt step

| craft | native | wheels | substeps | precalc | managed | native pre | Jolt step | native post | tick total | managed per wheel (us) | native per wheel (us) | wheel total per wheel (us) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | 8 | 8 | 0.0001 | 0.0035 | 0.0032 | 0.0063 | 0.0000 | 0.0131 | 0.44 | 0.40 | 0.84 |
| bug-e | explicit | 8 | 4 | 0.0001 | 0.0023 | 0.0032 | 0.0063 | 0.0000 | 0.0119 | 0.29 | 0.40 | 0.69 |
| bug-e | jolt-vc | 8 | 8 | 0.0001 | 0.0035 | 0.0001 | 0.0117 | 0.0001 | 0.0155 | 0.44 | 0.02 | 0.46 |
| bug-e | jolt-vc | 8 | 4 | 0.0001 | 0.0023 | 0.0001 | 0.0117 | 0.0001 | 0.0142 | 0.28 | 0.02 | 0.30 |
| prospector | explicit | 4 | 8 | 0.0001 | 0.0017 | 0.0017 | 0.0054 | 0.0000 | 0.0089 | 0.43 | 0.44 | 0.87 |
| prospector | explicit | 4 | 4 | 0.0001 | 0.0011 | 0.0017 | 0.0054 | 0.0000 | 0.0083 | 0.28 | 0.43 | 0.72 |
| prospector | jolt-vc | 4 | 8 | 0.0001 | 0.0016 | 0.0001 | 0.0082 | 0.0000 | 0.0101 | 0.41 | 0.03 | 0.44 |
| prospector | jolt-vc | 4 | 4 | 0.0001 | 0.0011 | 0.0001 | 0.0081 | 0.0001 | 0.0094 | 0.28 | 0.03 | 0.31 |
| crawler | explicit | 10 | 8 | 0.0001 | 0.0028 | 0.0031 | 0.0043 | 0.0000 | 0.0103 | 0.28 | 0.31 | 0.59 |
| crawler | explicit | 10 | 4 | 0.0001 | 0.0017 | 0.0031 | 0.0043 | 0.0000 | 0.0092 | 0.17 | 0.31 | 0.49 |
| crawler | jolt-vc | 10 | 8 | 0.0001 | 0.0028 | 0.0001 | 0.0101 | 0.0001 | 0.0131 | 0.28 | 0.02 | 0.30 |
| crawler | jolt-vc | 10 | 4 | 0.0001 | 0.0018 | 0.0001 | 0.0099 | 0.0001 | 0.0119 | 0.18 | 0.02 | 0.20 |
| twostage | explicit | 4 | 8 | 0.0001 | 0.0017 | 0.0017 | 0.0026 | 0.0000 | 0.0061 | 0.42 | 0.43 | 0.85 |
| twostage | explicit | 4 | 4 | 0.0001 | 0.0011 | 0.0017 | 0.0026 | 0.0000 | 0.0055 | 0.27 | 0.43 | 0.70 |
| twostage | jolt-vc | 4 | 8 | 0.0001 | 0.0017 | 0.0001 | 0.0056 | 0.0000 | 0.0075 | 0.42 | 0.03 | 0.45 |
| twostage | jolt-vc | 4 | 4 | 0.0001 | 0.0011 | 0.0001 | 0.0057 | 0.0000 | 0.0069 | 0.27 | 0.03 | 0.30 |
