### A. Headline numbers

| craft | window | parts | rigidbodies | attach joints (CJ) | all joints | colliders | FPS | frame ms mean | p50 | p95 | p99 | fixed/frame | sim rate (UT/wall) | GC/min | alloc MB/s (lower bound) | alt end m | srf speed end |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| run4-S | pad | 21 | 21 | 24 | 25 | 22 | 270.0 | 3.70 | 3.40 | 5.58 | 7.27 | 0.185 | 1.000 | 3.0 | 1.33 | 79 | 0 |
| run4-S | ascent | 21 | 21 | 24 | 24 | 22 | 216.7 | 4.61 | 3.66 | 8.20 | 11.49 | 0.231 | 1.000 | 2.0 | 1.24 | 2945 | 259 |
| run4-S-AS | pad | 21 | 21 | 24 | 45 | 22 | 276.6 | 3.62 | 3.29 | 5.35 | 6.88 | 0.181 | 1.000 | 3.0 | 1.36 | 79 | 0 |
| run4-S-AS | ascent | 21 | 21 | 24 | 44 | 22 | 190.9 | 5.24 | 4.23 | 9.01 | 11.65 | 0.262 | 1.000 | 2.0 | 1.09 | 2931 | 268 |
| run4-M2 | pad | 153 | 153 | 168 | 169 | 154 | 194.9 | 5.13 | 4.20 | 8.51 | 11.94 | 0.257 | 1.000 | 0.0 | 1.01 | 102 | 0 |
| run4-M2 | ascent | 153 | 153 | 168 | 168 | 154 | 115.2 | 8.68 | 5.98 | 15.87 | 19.73 | 0.434 | 1.000 | 2.0 | 0.81 | 6068 | 384 |
| run4-M2-AS | pad | 153 | 153 | 168 | 321 | 154 | 195.3 | 5.12 | 4.18 | 8.52 | 11.08 | 0.256 | 1.000 | 0.0 | 0.99 | 102 | 0 |
| run4-M2-AS | ascent | 153 | 153 | 168 | 320 | 154 | 112.8 | 8.86 | 6.08 | 16.07 | 19.59 | 0.443 | 1.000 | 2.0 | 0.78 | 6081 | 385 |
| run4-L2-AS | pad | 457 | 457 | 504 | 961 | 458 | 83.1 | 12.04 | 14.19 | 18.36 | 24.96 | 0.602 | 1.000 | 3.0 | 0.29 | 161 | 0 |
| run4-L2-AS | ascent | 457 | 457 | 504 | 960 | 458 | 26.2 | 38.15 | 39.58 | 54.47 | 67.01 | 1.739 | 0.912 | 0.0 | 0.72 | 2633 | 177 |
| run3-L2-AS | pad | 457 | 457 | 504 | 961 | 458 | 88.0 | 11.37 | 13.95 | 16.96 | 22.80 | 0.569 | 1.000 | 3.0 | 0.32 | 161 | 0 |
| run3-L2-AS | ascent | 457 | 457 | 504 | 960 | 458 | 26.0 | 38.38 | 39.38 | 54.79 | 66.83 | 1.769 | 0.921 | 0.0 | 0.69 | 2690 | 180 |

### B. Frame breakdown by Unity player-loop phase (ms per rendered frame, % of frame)

| phase | S pad | S ascent | S-AS pad | S-AS ascent | M2 pad | M2 ascent | M2-AS pad | M2-AS ascent | L2-AS pad | L2-AS ascent | L2-AS pad | L2-AS ascent |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| FixedUpdate scripts | 0.115 (3%) | 0.261 (6%) | 0.108 (3%) | 0.304 (6%) | 0.604 (12%) | 1.847 (21%) | 0.600 (12%) | 1.899 (21%) | 3.780 (31%) | 20.486 (54%) | 3.468 (31%) | 20.691 (54%) |
| PhysX step (+collision callbacks) | 0.093 (2%) | 0.398 (9%) | 0.089 (2%) | 0.467 (9%) | 0.205 (4%) | 0.950 (11%) | 0.250 (5%) | 1.057 (12%) | 1.187 (10%) | 6.957 (18%) | 1.118 (10%) | 7.011 (18%) |
| Update scripts | 0.652 (18%) | 0.720 (16%) | 0.830 (23%) | 1.014 (19%) | 1.234 (24%) | 1.523 (18%) | 1.213 (24%) | 1.531 (17%) | 3.021 (25%) | 4.180 (11%) | 2.921 (26%) | 4.196 (11%) |
| Rendering (cameras, renderers, present) | 0.891 (24%) | 1.178 (26%) | 0.823 (23%) | 1.251 (24%) | 0.989 (19%) | 1.653 (19%) | 0.981 (19%) | 1.672 (19%) | 1.307 (11%) | 2.736 (7%) | 1.249 (11%) | 2.706 (7%) |
| LateUpdate scripts | 0.589 (16%) | 0.620 (13%) | 0.532 (15%) | 0.637 (12%) | 0.692 (13%) | 0.846 (10%) | 0.694 (14%) | 0.856 (10%) | 0.997 (8%) | 1.228 (3%) | 0.954 (8%) | 1.200 (3%) |
| Coroutines (incl. PQS terrain) | 0.825 (22%) | 0.776 (17%) | 0.715 (20%) | 0.849 (16%) | 0.820 (16%) | 0.940 (11%) | 0.790 (15%) | 0.913 (10%) | 0.885 (7%) | 0.918 (2%) | 0.842 (7%) | 0.903 (2%) |
| UI canvases (uGUI) | 0.193 (5%) | 0.281 (6%) | 0.173 (5%) | 0.290 (6%) | 0.231 (4%) | 0.381 (4%) | 0.240 (5%) | 0.389 (4%) | 0.385 (3%) | 0.635 (2%) | 0.358 (3%) | 0.619 (2%) |
| Particles | 0.003 (0%) | 0.035 (1%) | 0.003 (0%) | 0.038 (1%) | 0.004 (0%) | 0.105 (1%) | 0.004 (0%) | 0.107 (1%) | 0.005 (0%) | 0.281 (1%) | 0.005 (0%) | 0.274 (1%) |
| FixedUpdate other | 0.003 (0%) | 0.009 (0%) | 0.003 (0%) | 0.010 (0%) | 0.005 (0%) | 0.035 (0%) | 0.004 (0%) | 0.036 (0%) | 0.014 (0%) | 0.252 (1%) | 0.013 (0%) | 0.260 (1%) |
| Other engine | 0.159 (4%) | 0.154 (3%) | 0.169 (5%) | 0.191 (4%) | 0.146 (3%) | 0.169 (2%) | 0.142 (3%) | 0.170 (2%) | 0.188 (2%) | 0.200 (1%) | 0.186 (2%) | 0.197 (1%) |
| Animation | 0.073 (2%) | 0.068 (1%) | 0.067 (2%) | 0.067 (1%) | 0.093 (2%) | 0.094 (1%) | 0.094 (2%) | 0.096 (1%) | 0.148 (1%) | 0.152 (0%) | 0.142 (1%) | 0.149 (0%) |
| Audio | 0.081 (2%) | 0.088 (2%) | 0.075 (2%) | 0.090 (2%) | 0.082 (2%) | 0.107 (1%) | 0.082 (2%) | 0.107 (1%) | 0.088 (1%) | 0.126 (0%) | 0.085 (1%) | 0.127 (0%) |
| Outside player loop (frame wait / present) | 0.027 (1%) | 0.028 (1%) | 0.029 (1%) | 0.031 (1%) | 0.027 (1%) | 0.029 (0%) | 0.027 (1%) | 0.029 (0%) | 0.027 (0%) | 0.031 (0%) | 0.026 (0%) | 0.064 (0%) |
| **total** | 3.704 | 4.615 | 3.616 | 5.239 | 5.132 | 8.680 | 5.120 | 8.863 | 12.031 | 38.182 | 11.367 | 38.396 |

### C. Physics-tick components (ms per physics tick; inclusive)

| component | S pad | S ascent | S-AS pad | S-AS ascent | M2 pad | M2 ascent | M2-AS pad | M2-AS ascent | L2-AS pad | L2-AS ascent | L2-AS pad | L2-AS ascent |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Planetarium.FixedUpdate (UT, body orbits) | 0.076 | 0.073 | 0.071 | 0.074 | 0.071 | 0.077 | 0.068 | 0.076 | 0.071 | 0.081 | 0.068 | 0.079 |
| VesselPrecalculate.FixedUpdate | 0.019 | 0.019 | 0.017 | 0.021 | 0.017 | 0.023 | 0.018 | 0.024 | 0.031 | 0.356 | 0.022 | 0.352 |
| OrbitDriver.FixedUpdate (all drivers) | 0.000 | 0.000 | 0.000 | 0.001 | 0.000 | 0.001 | 0.000 | 0.001 | 0.000 | 0.001 | 0.000 | 0.001 |
| CollisionEnhancer.FixedUpdate (per part) | 0.028 | 0.032 | 0.028 | 0.032 | 0.184 | 0.187 | 0.183 | 0.191 | 0.504 | 0.419 | 0.474 | 0.407 |
| FlightInputHandler.FixedUpdate (input, SAS/autopilot) | 0.012 | 0.047 | 0.021 | 0.055 | 0.049 | 0.173 | 0.047 | 0.173 | 0.123 | 0.444 | 0.117 | 0.439 |
| Part.FixedUpdate (per part, incl. OnFixedUpdate) | 0.062 | 0.060 | 0.057 | 0.061 | 0.251 | 0.271 | 0.240 | 0.268 | 0.708 | 0.710 | 0.699 | 0.733 |
| PartModule Unity FixedUpdates (sum) | 0.026 | 0.156 | 0.025 | 0.157 | 0.060 | 0.416 | 0.059 | 0.422 | 0.133 | 0.969 | 0.129 | 0.965 |
| PartBuoyancy.FixedUpdate (per part) | 0.039 | 0.038 | 0.035 | 0.038 | 0.242 | 0.248 | 0.232 | 0.250 | 0.681 | 0.663 | 0.674 | 0.671 |
| FlightIntegrator.FixedUpdate (total) | 0.214 | 0.199 | 0.211 | 0.210 | 1.207 | 1.167 | 1.231 | 1.209 | 3.448 | 3.293 | 3.349 | 3.255 |
|   FI.UpdateMassStats | 0.074 | 0.044 | 0.077 | 0.051 | 0.353 | 0.255 | 0.396 | 0.304 | 1.017 | 0.846 | 1.006 | 0.842 |
|   FI.DragCubeSetupAndPartAeroStats | 0.006 | 0.005 | 0.005 | 0.005 | 0.061 | 0.057 | 0.061 | 0.054 | 0.174 | 0.155 | 0.174 | 0.154 |
|   FI.CheckThermalGraph | 0.001 | 0.001 | 0.000 | 0.000 | 0.001 | 0.001 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
|   FI.UpdateThermodynamics | 0.040 | 0.055 | 0.038 | 0.056 | 0.242 | 0.269 | 0.241 | 0.269 | 0.630 | 0.650 | 0.625 | 0.667 |
|     of which UpdateConduction | 0.013 | 0.031 | 0.013 | 0.031 | 0.109 | 0.132 | 0.107 | 0.131 | 0.268 | 0.288 | 0.267 | 0.298 |
|   FI.Integrate (force application, self) | 0.021 | 0.019 | 0.018 | 0.019 | 0.143 | 0.155 | 0.139 | 0.152 | 0.409 | 0.435 | 0.392 | 0.421 |
|   FI.UpdateAerodynamics (drag, body lift) | 0.063 | 0.066 | 0.061 | 0.067 | 0.397 | 0.418 | 0.383 | 0.417 | 1.205 | 1.193 | 1.140 | 1.157 |
| Vessel.FixedUpdate | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 | 0.002 | 0.001 | 0.002 | 0.001 | 0.002 | 0.001 | 0.001 |
| Krakensbane.FixedUpdate | 0.003 | 0.008 | 0.002 | 0.008 | 0.003 | 0.034 | 0.003 | 0.031 | 0.003 | 0.093 | 0.003 | 0.091 |
| FloatingOrigin.FixedUpdate (incl. setOffset) | 0.001 | 0.347 | 0.001 | 0.348 | 0.001 | 1.240 | 0.001 | 1.224 | 0.001 | 3.324 | 0.001 | 3.326 |
| CollisionManager.FixedUpdate | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.002 | 0.000 | 0.001 | 0.000 | 0.018 | 0.000 | 0.000 |
| PQS.FixedUpdate | 0.010 | 0.010 | 0.008 | 0.010 | 0.010 | 0.012 | 0.010 | 0.013 | 0.013 | 0.015 | 0.011 | 0.015 |
| **All FixedUpdate scripts (measured slot)** | 0.620 | 1.130 | 0.599 | 1.160 | 2.353 | 4.256 | 2.343 | 4.287 | 6.280 | 11.779 | 6.097 | 11.699 |
| **PhysX simulate (PhysicsFixedUpdate slot minus callbacks)** | 0.498 | 1.725 | 0.491 | 1.781 | 0.798 | 2.190 | 0.973 | 2.385 | 1.969 | 4.000 | 1.964 | 3.964 |
| OnCollision*/OnJointBreak callbacks (inside PhysX slot) | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 |
| **Physics tick total (scripts + PhysX)** | 1.120 | 2.855 | 1.092 | 2.941 | 3.153 | 6.446 | 3.318 | 6.673 | 8.251 | 15.780 | 8.064 | 15.663 |

### D. Per-frame (Update/LateUpdate/coroutine) physics-adjacent work (ms per rendered frame)

| component | S pad | S ascent | S-AS pad | S-AS ascent | M2 pad | M2 ascent | M2-AS pad | M2-AS ascent | L2-AS pad | L2-AS ascent | L2-AS pad | L2-AS ascent |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| VesselPrecalculate.Update (CoM/MOI stats, orbit) | 0.014 | 0.016 | 0.012 | 0.019 | 0.057 | 0.103 | 0.056 | 0.105 | 0.368 | 0.731 | 0.325 | 0.728 |
|   of which CalculatePhysicsStats (all calls) | 0.007 | 0.008 | 0.007 | 0.010 | 0.047 | 0.085 | 0.046 | 0.087 | 0.342 | 1.209 | 0.297 | 1.220 |
| FlightIntegrator.Update (sun raycast, occlusion) | 0.044 | 0.043 | 0.037 | 0.044 | 0.091 | 0.131 | 0.089 | 0.135 | 0.376 | 0.909 | 0.357 | 0.901 |
| Part.Update (incl. OnUpdate) | 0.059 | 0.053 | 0.055 | 0.056 | 0.255 | 0.266 | 0.251 | 0.269 | 0.748 | 0.753 | 0.730 | 0.761 |
| PQS.UpdateQuads (coroutine) | 0.607 | 0.570 | 0.532 | 0.629 | 0.605 | 0.687 | 0.584 | 0.671 | 0.641 | 0.656 | 0.605 | 0.647 |
| PQS.UpdateVisual + Mod hooks (coroutine) | 0.150 | 0.142 | 0.124 | 0.152 | 0.147 | 0.175 | 0.140 | 0.166 | 0.167 | 0.173 | 0.161 | 0.176 |
| KSP/UI/* scripts (self time, all callbacks) | 0.267 | 0.330 | 0.244 | 0.338 | 0.298 | 0.421 | 0.294 | 0.420 | 0.379 | 0.529 | 0.367 | 0.530 |

### E. Top 20 instrumented methods by self time: run4-S pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.607 | 16.4 | 101.1 |
| SpeedDisplay.LateUpdate | 1.0 | 0.099 | 2.7 | 98.5 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.075 | 2.0 | 12.5 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.072 | 1.9 | 12.0 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.057 | 1.5 | 3.2 |
| CommNetNetwork.Update | 1.0 | 0.051 | 1.4 | 56.8 |
| TemperatureGaugeSystem.Update | 1.0 | 0.050 | 1.4 | 50.3 |
| ScaledSpace.LateUpdate | 1.0 | 0.044 | 1.2 | 44.1 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.039 | 1.0 | 38.8 |
| FlightCamera.LateUpdate | 1.0 | 0.038 | 1.0 | 38.5 |
| NavBall.Update | 1.0 | 0.029 | 0.8 | 29.0 |
| StageGroup.Update | 2.0 | 0.025 | 0.7 | 12.4 |
| ScaledSpaceFader.Update | 16.0 | 0.023 | 0.6 | 1.4 |
| SmokeTrailControl.Update | 12.0 | 0.023 | 0.6 | 1.9 |
| Part.ModulesOnUpdate | 21.0 | 0.022 | 0.6 | 1.6 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.020 | 0.6 | 20.4 |
| ProgressTracking.Update | 1.0 | 0.018 | 0.5 | 17.8 |
| VolumeNormalizer.Update | 1.0 | 0.018 | 0.5 | 17.6 |
| AtmosphereFromGround.LateUpdate | 5.0 | 0.017 | 0.5 | 3.5 |
| Part.Update | 21.0 | 0.016 | 0.4 | 2.8 |

### E. Top 20 instrumented methods by self time: run4-S ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.570 | 12.4 | 95.0 |
| NavBall.Update | 1.0 | 0.113 | 2.4 | 112.8 |
| SpeedDisplay.LateUpdate | 1.0 | 0.081 | 1.8 | 81.1 |
| FloatingOrigin.setOffset | 0.2 | 0.080 | 1.7 | 345.6 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.071 | 1.5 | 11.8 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.068 | 1.5 | 11.3 |
| OrbitRendererBase.LateUpdate | 18.1 | 0.058 | 1.3 | 3.2 |
| FlightCamera.LateUpdate | 1.0 | 0.052 | 1.1 | 52.0 |
| TemperatureGaugeSystem.Update | 1.0 | 0.048 | 1.0 | 47.6 |
| ScaledSpace.LateUpdate | 1.0 | 0.047 | 1.0 | 47.2 |
| CommNetNetwork.Update | 1.0 | 0.044 | 1.0 | 49.8 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.035 | 0.8 | 35.4 |
| ModuleEngines.FixedUpdate | 0.7 | 0.027 | 0.6 | 39.6 |
| ScaledSpaceFader.Update | 16.0 | 0.022 | 0.5 | 1.4 |
| StageGroup.Update | 2.0 | 0.022 | 0.5 | 10.8 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.021 | 0.5 | 21.3 |
| Part.ModulesOnUpdate | 21.0 | 0.021 | 0.4 | 1.5 |
| ProgressTracking.Update | 1.0 | 0.020 | 0.4 | 19.8 |
| SmokeTrailControl.Update | 12.0 | 0.019 | 0.4 | 1.6 |
| VolumeNormalizer.Update | 1.0 | 0.018 | 0.4 | 18.4 |

### E. Top 20 instrumented methods by self time: run4-S-AS pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.532 | 14.7 | 88.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.092 | 2.5 | 91.6 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.062 | 1.7 | 10.4 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.058 | 1.6 | 9.7 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.054 | 1.5 | 3.0 |
| TemperatureGaugeSystem.Update | 1.0 | 0.048 | 1.3 | 48.3 |
| CommNetNetwork.Update | 1.0 | 0.044 | 1.2 | 49.7 |
| ScaledSpace.LateUpdate | 1.0 | 0.041 | 1.1 | 40.7 |
| FlightCamera.LateUpdate | 1.0 | 0.039 | 1.1 | 38.6 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.031 | 0.9 | 31.4 |
| NavBall.Update | 1.0 | 0.026 | 0.7 | 26.0 |
| Part.ModulesOnUpdate | 21.0 | 0.022 | 0.6 | 1.5 |
| StageGroup.Update | 2.0 | 0.022 | 0.6 | 10.8 |
| SmokeTrailControl.Update | 12.0 | 0.020 | 0.6 | 1.7 |
| ScaledSpaceFader.Update | 16.0 | 0.020 | 0.5 | 1.2 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.018 | 0.5 | 18.2 |
| VolumeNormalizer.Update | 1.0 | 0.017 | 0.5 | 17.3 |
| ProgressTracking.Update | 1.0 | 0.017 | 0.5 | 16.6 |
| AtmosphereFromGround.LateUpdate | 5.0 | 0.017 | 0.5 | 3.3 |
| Part.Update | 21.0 | 0.015 | 0.4 | 2.6 |

### E. Top 20 instrumented methods by self time: run4-S-AS ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.629 | 12.0 | 104.8 |
| NavBall.Update | 1.0 | 0.114 | 2.2 | 114.1 |
| FloatingOrigin.setOffset | 0.3 | 0.091 | 1.7 | 347.4 |
| SpeedDisplay.LateUpdate | 1.0 | 0.087 | 1.7 | 86.8 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.076 | 1.5 | 12.7 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.072 | 1.4 | 11.9 |
| OrbitRendererBase.LateUpdate | 18.7 | 0.063 | 1.2 | 3.4 |
| FlightCamera.LateUpdate | 1.0 | 0.062 | 1.2 | 61.9 |
| ScaledSpace.LateUpdate | 1.0 | 0.049 | 0.9 | 49.4 |
| TemperatureGaugeSystem.Update | 1.0 | 0.049 | 0.9 | 49.0 |
| CommNetNetwork.Update | 1.0 | 0.044 | 0.8 | 50.0 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.035 | 0.7 | 34.7 |
| ModuleEngines.FixedUpdate | 0.8 | 0.031 | 0.6 | 40.0 |
| Part.ModulesOnUpdate | 21.0 | 0.023 | 0.4 | 1.6 |
| ScaledSpaceFader.Update | 16.0 | 0.022 | 0.4 | 1.4 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.021 | 0.4 | 20.8 |
| StageGroup.Update | 2.0 | 0.021 | 0.4 | 10.3 |
| VolumeNormalizer.Update | 1.0 | 0.020 | 0.4 | 20.3 |
| ProgressTracking.Update | 1.0 | 0.020 | 0.4 | 19.9 |
| SmokeTrailControl.Update | 12.0 | 0.019 | 0.4 | 1.6 |

### E. Top 20 instrumented methods by self time: run4-M2 pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.605 | 11.8 | 100.8 |
| Part.ModulesOnUpdate | 153.0 | 0.124 | 2.4 | 1.1 |
| FI.UpdateAerodynamics(per part) | 39.3 | 0.102 | 2.0 | 2.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.101 | 2.0 | 101.3 |
| SmokeTrailControl.Update | 68.0 | 0.098 | 1.9 | 1.4 |
| FI.UpdateMassStats | 0.3 | 0.091 | 1.8 | 353.3 |
| Part.Update | 153.0 | 0.085 | 1.7 | 1.7 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.075 | 1.5 | 12.5 |
| TemperatureGaugeSystem.Update | 1.0 | 0.073 | 1.4 | 73.1 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.069 | 1.3 | 11.5 |
| CommNetNetwork.Update | 1.0 | 0.068 | 1.3 | 74.6 |
| Part.FixedUpdate | 39.3 | 0.064 | 1.3 | 1.6 |
| PartBuoyancy.FixedUpdate | 39.3 | 0.062 | 1.2 | 1.6 |
| VolumeNormalizer.Update | 1.0 | 0.059 | 1.1 | 58.9 |
| FlightCamera.LateUpdate | 1.0 | 0.056 | 1.1 | 56.0 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.055 | 1.1 | 3.1 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.049 | 1.0 | 49.0 |
| VesselPrecalculate.CalculatePhysicsStats | 0.5 | 0.047 | 0.9 | 92.3 |
| CollisionEnhancer.FixedUpdate | 39.3 | 0.047 | 0.9 | 1.2 |
| ScaledSpace.LateUpdate | 1.0 | 0.047 | 0.9 | 46.7 |

### E. Top 20 instrumented methods by self time: run4-M2 ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.687 | 7.9 | 114.5 |
| FloatingOrigin.setOffset | 0.4 | 0.537 | 6.2 | 1309.0 |
| FI.UpdateAerodynamics(per part) | 66.4 | 0.181 | 2.1 | 2.7 |
| ModuleEngines.FixedUpdate | 7.4 | 0.144 | 1.7 | 19.5 |
| Part.ModulesOnUpdate | 153.0 | 0.133 | 1.5 | 1.1 |
| NavBall.Update | 1.0 | 0.127 | 1.5 | 127.0 |
| FI.UpdateMassStats | 0.4 | 0.111 | 1.3 | 255.2 |
| PartBuoyancy.FixedUpdate | 66.4 | 0.108 | 1.2 | 1.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.107 | 1.2 | 106.6 |
| Part.FixedUpdate | 66.4 | 0.100 | 1.2 | 1.8 |
| SmokeTrailControl.Update | 68.0 | 0.094 | 1.1 | 1.4 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.088 | 1.0 | 14.7 |
| VesselPrecalculate.CalculatePhysicsStats | 1.2 | 0.085 | 1.0 | 69.5 |
| Part.Update | 153.0 | 0.085 | 1.0 | 1.7 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.083 | 1.0 | 13.8 |
| CollisionEnhancer.FixedUpdate | 66.4 | 0.081 | 0.9 | 1.2 |
| FlightCamera.LateUpdate | 1.0 | 0.078 | 0.9 | 77.9 |
| TemperatureGaugeSystem.Update | 1.0 | 0.078 | 0.9 | 77.9 |
| FlightInputHandler.FixedUpdate | 0.4 | 0.075 | 0.9 | 172.9 |
| FI.UpdateOcclusion(partial,Update) | 0.2 | 0.071 | 0.8 | 352.1 |

### E. Top 20 instrumented methods by self time: run4-M2-AS pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.584 | 11.4 | 97.3 |
| Part.ModulesOnUpdate | 153.0 | 0.124 | 2.4 | 1.0 |
| FI.UpdateMassStats | 0.3 | 0.101 | 2.0 | 396.0 |
| SpeedDisplay.LateUpdate | 1.0 | 0.100 | 2.0 | 100.1 |
| SmokeTrailControl.Update | 68.0 | 0.099 | 1.9 | 1.5 |
| FI.UpdateAerodynamics(per part) | 39.2 | 0.098 | 1.9 | 2.5 |
| Part.Update | 153.0 | 0.083 | 1.6 | 1.6 |
| TemperatureGaugeSystem.Update | 1.0 | 0.073 | 1.4 | 73.3 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.072 | 1.4 | 11.9 |
| CommNetNetwork.Update | 1.0 | 0.067 | 1.3 | 73.1 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.065 | 1.3 | 10.9 |
| Part.FixedUpdate | 39.2 | 0.062 | 1.2 | 1.6 |
| PartBuoyancy.FixedUpdate | 39.2 | 0.059 | 1.2 | 1.5 |
| VolumeNormalizer.Update | 1.0 | 0.058 | 1.1 | 58.2 |
| FlightCamera.LateUpdate | 1.0 | 0.055 | 1.1 | 55.0 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.054 | 1.1 | 3.0 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.048 | 0.9 | 47.8 |
| CollisionEnhancer.FixedUpdate | 39.2 | 0.047 | 0.9 | 1.2 |
| ScaledSpace.LateUpdate | 1.0 | 0.046 | 0.9 | 46.5 |
| VesselPrecalculate.CalculatePhysicsStats | 0.5 | 0.046 | 0.9 | 90.0 |

### E. Top 20 instrumented methods by self time: run4-M2-AS ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.671 | 7.6 | 111.9 |
| FloatingOrigin.setOffset | 0.4 | 0.542 | 6.1 | 1291.5 |
| FI.UpdateAerodynamics(per part) | 67.8 | 0.185 | 2.1 | 2.7 |
| ModuleEngines.FixedUpdate | 7.5 | 0.149 | 1.7 | 19.8 |
| Part.ModulesOnUpdate | 153.0 | 0.139 | 1.6 | 1.2 |
| FI.UpdateMassStats | 0.4 | 0.135 | 1.5 | 303.9 |
| NavBall.Update | 1.0 | 0.126 | 1.4 | 126.2 |
| PartBuoyancy.FixedUpdate | 67.8 | 0.111 | 1.3 | 1.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.105 | 1.2 | 105.3 |
| Part.FixedUpdate | 67.8 | 0.102 | 1.1 | 1.8 |
| SmokeTrailControl.Update | 68.0 | 0.096 | 1.1 | 1.4 |
| VesselPrecalculate.CalculatePhysicsStats | 1.2 | 0.087 | 1.0 | 73.4 |
| Part.Update | 153.0 | 0.085 | 1.0 | 1.8 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.085 | 1.0 | 14.1 |
| CollisionEnhancer.FixedUpdate | 67.8 | 0.084 | 1.0 | 1.2 |
| TemperatureGaugeSystem.Update | 1.0 | 0.079 | 0.9 | 79.5 |
| FlightCamera.LateUpdate | 1.0 | 0.078 | 0.9 | 78.4 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.077 | 0.9 | 12.9 |
| FlightInputHandler.FixedUpdate | 0.4 | 0.077 | 0.9 | 173.2 |
| FI.UpdateOcclusion(partial,Update) | 0.2 | 0.073 | 0.8 | 355.7 |

### E. Top 20 instrumented methods by self time: run4-L2-AS pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| FI.UpdateAerodynamics(per part) | 274.8 | 0.725 | 6.0 | 2.6 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.641 | 5.3 | 106.8 |
| FI.UpdateMassStats | 0.6 | 0.612 | 5.1 | 1018.0 |
| Part.FixedUpdate | 274.8 | 0.426 | 3.5 | 1.5 |
| PartBuoyancy.FixedUpdate | 274.8 | 0.410 | 3.4 | 1.5 |
| Part.ModulesOnUpdate | 457.0 | 0.402 | 3.3 | 1.1 |
| VesselPrecalculate.CalculatePhysicsStats | 1.2 | 0.342 | 2.8 | 284.1 |
| CollisionEnhancer.FixedUpdate | 274.8 | 0.303 | 2.5 | 1.1 |
| FI.UpdateOcclusion(partial,Update) | 0.3 | 0.286 | 2.4 | 1056.4 |
| SmokeTrailControl.Update | 196.0 | 0.275 | 2.3 | 1.4 |
| FI.Integrate(root) | 0.6 | 0.246 | 2.0 | 1615.6 |
| Part.Update | 457.0 | 0.237 | 2.0 | 1.6 |
| FI.UpdateThermodynamics | 0.6 | 0.195 | 1.6 | 630.2 |
| FI.UpdateConduction | 0.6 | 0.161 | 1.3 | 268.1 |
| CommNetNetwork.Update | 1.0 | 0.156 | 1.3 | 165.3 |
| VolumeNormalizer.Update | 1.0 | 0.148 | 1.2 | 148.5 |
| Part.LateUpdate | 457.0 | 0.137 | 1.1 | 0.3 |
| TemperatureGaugeSystem.Update | 1.0 | 0.131 | 1.1 | 131.4 |
| FI.DragCubeSetupAndPartAeroStats | 0.6 | 0.105 | 0.9 | 174.5 |
| SpeedDisplay.LateUpdate | 1.0 | 0.105 | 0.9 | 104.9 |

### E. Top 20 instrumented methods by self time: run4-L2-AS ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| FloatingOrigin.setOffset | 1.6 | 5.776 | 15.1 | 3549.7 |
| FI.UpdateAerodynamics(per part) | 795.4 | 2.075 | 5.4 | 2.6 |
| FI.UpdateMassStats | 1.7 | 1.471 | 3.9 | 845.2 |
| VesselDeltaV.FixedUpdate | 1.7 | 1.396 | 3.7 | 802.0 |
| ModuleEngines.FixedUpdate | 85.3 | 1.356 | 3.6 | 15.9 |
| VesselPrecalculate.CalculatePhysicsStats | 4.7 | 1.209 | 3.2 | 258.3 |
| PartBuoyancy.FixedUpdate | 795.4 | 1.153 | 3.0 | 1.4 |
| Part.FixedUpdate | 795.4 | 1.048 | 2.7 | 1.6 |
| FI.UpdateOcclusion(partial,Update) | 0.7 | 0.783 | 2.1 | 1083.4 |
| FlightInputHandler.FixedUpdate | 1.7 | 0.773 | 2.0 | 443.9 |
| FI.Integrate(root) | 1.7 | 0.757 | 2.0 | 1627.4 |
| CollisionEnhancer.FixedUpdate | 795.4 | 0.729 | 1.9 | 0.9 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.656 | 1.7 | 109.4 |
| FI.UpdateThermodynamics | 1.7 | 0.563 | 1.5 | 649.6 |
| FI.UpdateConduction | 1.7 | 0.501 | 1.3 | 287.9 |
| Part.ModulesOnUpdate | 457.0 | 0.410 | 1.1 | 1.1 |
| SmokeTrailControl.Update | 196.0 | 0.270 | 0.7 | 1.4 |
| FI.DragCubeSetupAndPartAeroStats | 1.7 | 0.269 | 0.7 | 154.4 |
| Part.Update | 457.0 | 0.235 | 0.6 | 1.6 |
| VolumeNormalizer.Update | 1.0 | 0.153 | 0.4 | 153.3 |

### E. Top 20 instrumented methods by self time: run3-L2-AS pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| FI.UpdateAerodynamics(per part) | 259.7 | 0.649 | 5.7 | 2.5 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.605 | 5.3 | 100.9 |
| FI.UpdateMassStats | 0.6 | 0.572 | 5.0 | 1006.9 |
| Part.FixedUpdate | 259.7 | 0.398 | 3.5 | 1.5 |
| Part.ModulesOnUpdate | 457.0 | 0.396 | 3.5 | 1.1 |
| PartBuoyancy.FixedUpdate | 259.7 | 0.383 | 3.4 | 1.5 |
| VesselPrecalculate.CalculatePhysicsStats | 1.1 | 0.297 | 2.6 | 261.8 |
| SmokeTrailControl.Update | 196.0 | 0.276 | 2.4 | 1.4 |
| FI.UpdateOcclusion(partial,Update) | 0.3 | 0.272 | 2.4 | 1051.9 |
| CollisionEnhancer.FixedUpdate | 259.7 | 0.270 | 2.4 | 1.0 |
| Part.Update | 457.0 | 0.229 | 2.0 | 1.6 |
| FI.Integrate(root) | 0.6 | 0.223 | 2.0 | 1533.6 |
| FI.UpdateThermodynamics | 0.6 | 0.184 | 1.6 | 625.1 |
| FI.UpdateConduction | 0.6 | 0.152 | 1.3 | 267.3 |
| VolumeNormalizer.Update | 1.0 | 0.148 | 1.3 | 148.1 |
| CommNetNetwork.Update | 1.0 | 0.148 | 1.3 | 155.7 |
| Part.LateUpdate | 457.0 | 0.130 | 1.1 | 0.3 |
| TemperatureGaugeSystem.Update | 1.0 | 0.126 | 1.1 | 126.4 |
| SpeedDisplay.LateUpdate | 1.0 | 0.103 | 0.9 | 103.3 |
| FI.DragCubeSetupAndPartAeroStats | 0.6 | 0.099 | 0.9 | 174.6 |

### E. Top 20 instrumented methods by self time: run3-L2-AS ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| FloatingOrigin.setOffset | 1.7 | 5.878 | 15.3 | 3549.5 |
| FI.UpdateAerodynamics(per part) | 808.2 | 2.046 | 5.3 | 2.5 |
| FI.UpdateMassStats | 1.8 | 1.489 | 3.9 | 841.8 |
| VesselDeltaV.FixedUpdate | 1.8 | 1.372 | 3.6 | 775.5 |
| ModuleEngines.FixedUpdate | 86.7 | 1.371 | 3.6 | 15.8 |
| VesselPrecalculate.CalculatePhysicsStats | 3.5 | 1.220 | 3.2 | 344.9 |
| PartBuoyancy.FixedUpdate | 808.2 | 1.186 | 3.1 | 1.5 |
| Part.FixedUpdate | 808.2 | 1.101 | 2.9 | 1.6 |
| FI.UpdateOcclusion(partial,Update) | 0.7 | 0.779 | 2.0 | 1070.7 |
| FlightInputHandler.FixedUpdate | 1.8 | 0.776 | 2.0 | 438.7 |
| FI.Integrate(root) | 1.8 | 0.744 | 1.9 | 1577.6 |
| CollisionEnhancer.FixedUpdate | 808.2 | 0.719 | 1.9 | 0.9 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.647 | 1.7 | 107.9 |
| FI.UpdateThermodynamics | 1.8 | 0.588 | 1.5 | 667.4 |
| FI.UpdateConduction | 1.8 | 0.528 | 1.4 | 298.5 |
| Part.ModulesOnUpdate | 457.0 | 0.414 | 1.1 | 1.1 |
| SmokeTrailControl.Update | 196.0 | 0.275 | 0.7 | 1.4 |
| FI.DragCubeSetupAndPartAeroStats | 1.8 | 0.272 | 0.7 | 153.7 |
| Part.Update | 457.0 | 0.238 | 0.6 | 1.7 |
| VolumeNormalizer.Update | 1.0 | 0.156 | 0.4 | 155.7 |

### F. PartModule work per physics tick (ascent; inclusive ms/tick, calls/tick)

| module method | S ms/tick (calls/tick) | S-AS ms/tick (calls/tick) | M2 ms/tick (calls/tick) | M2-AS ms/tick (calls/tick) | L2-AS ms/tick (calls/tick) | L2-AS ms/tick (calls/tick) |
|---|---|---|---|---|---|---|
| ModuleEngines.FixedUpdate | 0.1188 (3.0) | 0.1199 (3.0) | 0.3321 (17.0) | 0.3364 (17.0) | 0.7798 (49.0) | 0.7754 (49.0) |
| FXModuleAnimateThrottle.FixedUpdate | 0.0086 (2.0) | 0.0087 (2.0) | 0.0292 (16.0) | 0.0299 (16.0) | 0.0706 (48.0) | 0.0699 (48.0) |
| ModuleGimbal.FixedUpdate | 0.0068 (3.0) | 0.0066 (3.0) | 0.0239 (17.0) | 0.0238 (17.0) | 0.0625 (49.0) | 0.0637 (49.0) |
| ModuleAlternator.FixedUpdate | 0.0060 (3.0) | 0.0055 (3.0) | 0.0161 (17.0) | 0.0165 (17.0) | 0.0396 (49.0) | 0.0393 (49.0) |
| ModuleCommand.FixedUpdate | 0.0123 (1.0) | 0.0118 (1.0) | 0.0116 (1.0) | 0.0117 (1.0) | 0.0121 (1.0) | 0.0122 (1.0) |
| ModuleReactionWheel.FixedUpdate | 0.0036 (1.0) | 0.0042 (1.0) | 0.0032 (1.0) | 0.0035 (1.0) | 0.0038 (1.0) | 0.0037 (1.0) |
| ModuleCargoPart.OnFixedUpdate | 0.0003 (2.0) | 0.0003 (2.0) | 0.0013 (16.0) | 0.0013 (16.0) | 0.0039 (48.0) | 0.0041 (48.0) |
| FXModuleAnimateThrottle.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0013 (17.0) | 0.0013 (17.0) | 0.0030 (49.0) | 0.0035 (49.0) |
| ModuleGimbal.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0009 (17.0) | 0.0009 (17.0) | 0.0029 (49.0) | 0.0027 (49.0) |
| ModuleSurfaceFX.OnFixedUpdate | 0.0002 (3.0) | 0.0003 (3.0) | 0.0011 (17.0) | 0.0009 (17.0) | 0.0023 (49.0) | 0.0027 (49.0) |
| ModuleAlternator.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0009 (17.0) | 0.0009 (17.0) | 0.0024 (49.0) | 0.0026 (49.0) |
| ModuleJettison.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0009 (17.0) | 0.0009 (17.0) | 0.0023 (49.0) | 0.0024 (49.0) |
| ModuleEngines.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0008 (17.0) | 0.0009 (17.0) | 0.0023 (49.0) | 0.0024 (49.0) |
| ModuleTestSubject.OnFixedUpdate | 0.0003 (3.0) | 0.0003 (3.0) | 0.0009 (17.0) | 0.0009 (17.0) | 0.0022 (49.0) | 0.0024 (49.0) |
| ModuleSAS.FixedUpdate | 0.0003 (1.0) | 0.0003 (1.0) | 0.0003 (1.0) | 0.0003 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) |
| ModulePartVariants.OnFixedUpdate | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) |
| FXModuleLookAtConstraint.OnFixedUpdate | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0001 (1.0) | 0.0002 (1.0) |
| FXModuleLookAtConstraint.FixedUpdate | 0.0000 (1.0) | 0.0000 (1.0) | 0.0000 (1.0) | 0.0000 (1.0) | 0.0000 (1.0) | 0.0001 (1.0) |

### G. Instrumentation overhead estimate

| run | instrumented calls/frame | pair cost ns | est. overhead ms/frame | % of frame |
|---|---|---|---|---|
| run4-S pad | 624 | 49.2 | 0.031 | 0.8% |
| run4-S ascent | 644 | 49.2 | 0.032 | 0.7% |
| run4-S-AS pad | 615 | 48.9 | 0.030 | 0.8% |
| run4-S-AS ascent | 649 | 48.9 | 0.032 | 0.6% |
| run4-M2 pad | 1697 | 49.3 | 0.084 | 1.6% |
| run4-M2 ascent | 1919 | 49.3 | 0.095 | 1.1% |
| run4-M2-AS pad | 1697 | 51.2 | 0.087 | 1.7% |
| run4-M2-AS ascent | 1926 | 51.2 | 0.099 | 1.1% |
| run4-L2-AS pad | 4870 | 49.0 | 0.238 | 2.0% |
| run4-L2-AS ascent | 8118 | 49.0 | 0.397 | 1.0% |
| run3-L2-AS pad | 4790 | 49.5 | 0.237 | 2.1% |
| run3-L2-AS ascent | 8169 | 49.5 | 0.404 | 1.1% |

### H. PhysX and FixedUpdate-script cost per tick, split by whether the tick had a floating-origin shift (ascent windows)

| run | ticks without shift | PhysX ms/tick | scripts ms/tick | ticks with shift | PhysX ms/tick | scripts ms/tick |
|---|---|---|---|---|---|---|
| run4-S | 0 | - | - | 1500 | 1.73 | 1.13 |
| run4-S-AS | 0 | - | - | 1501 | 1.78 | 1.16 |
| run4-M2 | 81 | 0.71 | 3.05 | 1419 | 2.27 | 4.33 |
| run4-M2-AS | 81 | 0.90 | 2.96 | 1419 | 2.47 | 4.36 |
| run4-L2-AS | 90 | 1.89 | 8.82 | 1277 | 4.14 | 11.97 |
| run3-L2-AS | 90 | 2.01 | 9.83 | 1293 | 4.10 | 11.82 |

### I. Frames containing a GC collection

| run | window | median frame ms | GC frames (frame ms) | managed heap MB (min-max) |
|---|---|---|---|---|
| run4-S | pad | 3.40 | 12 | 641-669 |
| run4-S | ascent | 3.66 | 11 | 646-687 |
| run4-S-AS | pad | 3.29 | 11 | 641-668 |
| run4-S-AS | ascent | 4.23 | 12 | 646-687 |
| run4-M2 | pad | 4.20 | none | 666-687 |
| run4-M2 | ascent | 5.98 | 20 | 672-711 |
| run4-M2-AS | pad | 4.18 | none | 666-687 |
| run4-M2-AS | ascent | 6.08 | 12 | 672-711 |
| run4-L2-AS | pad | 14.19 | 23 | 721-750 |
| run4-L2-AS | ascent | 39.58 | none | 742-765 |
| run3-L2-AS | pad | 13.95 | 25 | 722-750 |
| run3-L2-AS | ascent | 39.40 | none | 743-765 |

