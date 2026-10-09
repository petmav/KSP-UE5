### A. Headline numbers

| craft | window | parts | rigidbodies | attach joints (CJ) | all joints | colliders | FPS | frame ms mean | p50 | p95 | p99 | fixed/frame | sim rate (UT/wall) | GC/min | alloc MB/s (lower bound) | alt end m | srf speed end |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| run1-S | pad | 21 | 21 | 24 | 25 | 22 | 254.6 | 3.93 | 3.60 | 6.26 | 8.10 | 0.196 | 1.001 | 0.0 | 1.33 | 79 | 0 |
| run1-S | ascent | 21 | 21 | 24 | 24 | 22 | 172.8 | 5.79 | 4.85 | 9.72 | 12.76 | 0.289 | 1.000 | 2.0 | 1.04 | 2939 | 258 |
| run2-M2 | pad | 153 | 153 | 168 | 169 | 154 | 192.6 | 5.19 | 4.30 | 8.28 | 10.74 | 0.260 | 1.000 | 3.0 | 0.91 | 102 | 0 |
| run2-M2 | ascent | 153 | 153 | 168 | 168 | 154 | 114.8 | 8.71 | 5.92 | 15.88 | 19.54 | 0.436 | 1.000 | 2.0 | 0.81 | 6068 | 384 |
| run1-M | pad | 150 | 150 | 159 | 160 | 151 | 218.6 | 4.57 | 3.87 | 7.58 | 8.98 | 0.229 | 1.000 | 0.0 | 1.09 | 90 | 0 |
| run1-M | ascent | 150 | 150 | 159 | 159 | 151 | 123.4 | 8.10 | 5.58 | 13.71 | 17.56 | 0.405 | 1.000 | 2.0 | 0.83 | 5057 | 332 |
| run1-L | pad | 446 | 446 | 471 | 472 | 447 | 92.2 | 10.85 | 12.95 | 17.54 | 24.33 | 0.543 | 1.000 | 3.0 | 0.34 | 120 | 0 |
| run1-L | ascent | 234 | 234 | 259 | 259 | 235 | 45.3 | 22.08 | 19.04 | 49.28 | 65.22 | 1.046 | 0.948 | 14.0 | 20.36 | 120 | 0 |

### B. Frame breakdown by Unity player-loop phase (ms per rendered frame, % of frame)

| phase | S pad | S ascent | M2 pad | M2 ascent | M pad | M ascent | L pad | L ascent |
|---|---|---|---|---|---|---|---|---|
| FixedUpdate scripts | 0.122 (3%) | 0.353 (6%) | 0.575 (11%) | 1.775 (20%) | 0.477 (10%) | 1.581 (20%) | 3.299 (30%) | 8.693 (39%) |
| Update scripts | 0.921 (23%) | 1.050 (18%) | 1.491 (29%) | 1.884 (22%) | 1.126 (25%) | 1.484 (18%) | 2.864 (26%) | 3.292 (15%) |
| UI canvases (uGUI) | 0.188 (5%) | 0.315 (5%) | 0.218 (4%) | 0.362 (4%) | 0.212 (5%) | 0.485 (6%) | 0.335 (3%) | 2.152 (10%) |
| Rendering (cameras, renderers, present) | 0.880 (22%) | 1.364 (24%) | 0.951 (18%) | 1.600 (18%) | 0.899 (20%) | 1.566 (19%) | 1.274 (12%) | 2.086 (9%) |
| Coroutines (incl. PQS terrain) | 0.788 (20%) | 0.989 (17%) | 0.751 (14%) | 0.862 (10%) | 0.731 (16%) | 0.876 (11%) | 0.870 (8%) | 1.676 (8%) |
| PhysX step (+collision callbacks) | 0.096 (2%) | 0.528 (9%) | 0.201 (4%) | 0.926 (11%) | 0.168 (4%) | 0.809 (10%) | 0.787 (7%) | 1.399 (6%) |
| LateUpdate scripts | 0.575 (15%) | 0.712 (12%) | 0.628 (12%) | 0.766 (9%) | 0.636 (14%) | 0.787 (10%) | 0.959 (9%) | 1.252 (6%) |
| FixedUpdate other | 0.003 (0%) | 0.014 (0%) | 0.004 (0%) | 0.033 (0%) | 0.003 (0%) | 0.030 (0%) | 0.012 (0%) | 0.414 (2%) |
| Particles | 0.003 (0%) | 0.046 (1%) | 0.004 (0%) | 0.099 (1%) | 0.004 (0%) | 0.103 (1%) | 0.004 (0%) | 0.393 (2%) |
| Other engine | 0.178 (5%) | 0.201 (3%) | 0.174 (3%) | 0.187 (2%) | 0.135 (3%) | 0.160 (2%) | 0.178 (2%) | 0.351 (2%) |
| Audio | 0.074 (2%) | 0.094 (2%) | 0.077 (1%) | 0.102 (1%) | 0.079 (2%) | 0.103 (1%) | 0.086 (1%) | 0.168 (1%) |
| Animation | 0.071 (2%) | 0.075 (1%) | 0.089 (2%) | 0.091 (1%) | 0.087 (2%) | 0.092 (1%) | 0.144 (1%) | 0.155 (1%) |
| Outside player loop (frame wait / present) | 0.027 (1%) | 0.046 (1%) | 0.028 (1%) | 0.028 (0%) | 0.018 (0%) | 0.025 (0%) | 0.030 (0%) | 0.036 (0%) |
| **total** | 3.928 | 5.788 | 5.192 | 8.714 | 4.574 | 8.101 | 10.843 | 22.067 |

### C. Physics-tick components (ms per physics tick; inclusive)

| component | S pad | S ascent | M2 pad | M2 ascent | M pad | M ascent | L pad | L ascent |
|---|---|---|---|---|---|---|---|---|
| Planetarium.FixedUpdate (UT, body orbits) | 0.073 | 0.079 | 0.067 | 0.073 | 0.066 | 0.073 | 0.072 | 0.084 |
| VesselPrecalculate.FixedUpdate | 0.019 | 0.025 | 0.017 | 0.020 | 0.017 | 0.019 | 0.030 | 0.127 |
| OrbitDriver.FixedUpdate (all drivers) | 0.001 | 0.001 | 0.000 | 0.001 | 0.000 | 0.000 | 0.000 | 0.001 |
| CollisionEnhancer.FixedUpdate (per part) | 0.028 | 0.034 | 0.179 | 0.182 | 0.178 | 0.185 | 0.482 | 0.337 |
| FlightInputHandler.FixedUpdate (input, SAS/autopilot) | 0.023 | 0.057 | 0.047 | 0.166 | 0.044 | 0.162 | 0.116 | 0.369 |
| Part.FixedUpdate (per part, incl. OnFixedUpdate) | 0.060 | 0.066 | 0.231 | 0.256 | 0.221 | 0.249 | 0.682 | 1.747 |
| PartModule Unity FixedUpdates (sum) | 0.026 | 0.171 | 0.058 | 0.404 | 0.058 | 0.421 | 0.131 | 1.385 |
| PartBuoyancy.FixedUpdate (per part) | 0.039 | 0.040 | 0.225 | 0.238 | 0.213 | 0.231 | 0.713 | 0.570 |
| FlightIntegrator.FixedUpdate (total) | 0.210 | 0.213 | 1.132 | 1.104 | 1.047 | 1.038 | 3.300 | 2.931 |
|   FI.UpdateMassStats | 0.071 | 0.045 | 0.327 | 0.241 | 0.306 | 0.219 | 0.891 | 0.666 |
|   FI.DragCubeSetupAndPartAeroStats | 0.006 | 0.005 | 0.053 | 0.049 | 0.044 | 0.042 | 0.188 | 0.193 |
|   FI.CheckThermalGraph | 0.001 | 0.001 | 0.000 | 0.000 | 0.001 | 0.001 | 0.000 | 0.213 |
|   FI.UpdateThermodynamics | 0.039 | 0.060 | 0.231 | 0.257 | 0.214 | 0.244 | 0.633 | 0.439 |
|     of which UpdateConduction | 0.013 | 0.034 | 0.103 | 0.125 | 0.093 | 0.117 | 0.277 | 0.197 |
|   FI.Integrate (force application, self) | 0.020 | 0.021 | 0.135 | 0.144 | 0.120 | 0.136 | 0.407 | 0.326 |
|   FI.UpdateAerodynamics (drag, body lift) | 0.063 | 0.071 | 0.376 | 0.402 | 0.351 | 0.386 | 1.167 | 0.844 |
| Vessel.FixedUpdate | 0.001 | 0.002 | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 | 0.005 |
| Krakensbane.FixedUpdate | 0.003 | 0.009 | 0.002 | 0.031 | 0.003 | 0.028 | 0.003 | 0.003 |
| FloatingOrigin.FixedUpdate (incl. setOffset) | 0.001 | 0.355 | 0.001 | 1.199 | 0.001 | 1.121 | 0.001 | 0.001 |
| CollisionManager.FixedUpdate | 0.000 | 0.000 | 0.000 | 0.001 | 0.000 | 0.000 | 0.000 | 0.002 |
| PQS.FixedUpdate | 0.009 | 0.013 | 0.009 | 0.011 | 0.009 | 0.011 | 0.012 | 0.016 |
| **All FixedUpdate scripts (measured slot)** | 0.622 | 1.222 | 2.214 | 4.074 | 2.088 | 3.905 | 6.081 | 8.308 |
| **PhysX simulate (PhysicsFixedUpdate slot minus callbacks)** | 0.486 | 1.826 | 0.772 | 2.126 | 0.731 | 1.998 | 1.449 | 1.335 |
| OnCollision*/OnJointBreak callbacks (inside PhysX slot) | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.000 | 0.002 | 0.002 |
| **Physics tick total (scripts + PhysX)** | 1.110 | 3.048 | 2.988 | 6.199 | 2.820 | 5.903 | 7.531 | 9.645 |

### D. Per-frame (Update/LateUpdate/coroutine) physics-adjacent work (ms per rendered frame)

| component | S pad | S ascent | M2 pad | M2 ascent | M pad | M ascent | L pad | L ascent |
|---|---|---|---|---|---|---|---|---|
| VesselPrecalculate.Update (CoM/MOI stats, orbit) | 0.013 | 0.024 | 0.055 | 0.100 | 0.045 | 0.087 | 0.313 | 0.379 |
|   of which CalculatePhysicsStats (all calls) | 0.008 | 0.013 | 0.047 | 0.084 | 0.037 | 0.072 | 0.292 | 0.441 |
| FlightIntegrator.Update (sun raycast, occlusion) | 0.042 | 0.055 | 0.084 | 0.126 | 0.077 | 0.114 | 0.320 | 0.400 |
| Part.Update (incl. OnUpdate) | 0.056 | 0.060 | 0.238 | 0.253 | 0.231 | 0.257 | 0.713 | 0.559 |
| PQS.UpdateQuads (coroutine) | 0.579 | 0.722 | 0.554 | 0.634 | 0.540 | 0.647 | 0.629 | 0.633 |
| PQS.UpdateVisual + Mod hooks (coroutine) | 0.140 | 0.183 | 0.134 | 0.157 | 0.129 | 0.158 | 0.165 | 0.171 |
| KSP/UI/* scripts (self time, all callbacks) | 0.256 | 0.387 | 0.281 | 0.395 | 0.278 | 0.444 | 0.370 | 0.740 |

### E. Top 20 instrumented methods by self time: run1-S pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.579 | 14.8 | 96.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.095 | 2.4 | 95.1 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.072 | 1.8 | 12.0 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.065 | 1.7 | 10.8 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.054 | 1.4 | 3.0 |
| TemperatureGaugeSystem.Update | 1.0 | 0.049 | 1.2 | 48.9 |
| CommNetNetwork.Update | 1.0 | 0.046 | 1.2 | 52.3 |
| ScaledSpace.LateUpdate | 1.0 | 0.044 | 1.1 | 44.4 |
| FlightCamera.LateUpdate | 1.0 | 0.043 | 1.1 | 42.5 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.036 | 0.9 | 35.9 |
| NavBall.Update | 1.0 | 0.028 | 0.7 | 27.6 |
| StageGroup.Update | 2.0 | 0.023 | 0.6 | 11.5 |
| Part.ModulesOnUpdate | 21.0 | 0.022 | 0.6 | 1.5 |
| ScaledSpaceFader.Update | 16.0 | 0.022 | 0.6 | 1.4 |
| SmokeTrailControl.Update | 12.0 | 0.021 | 0.5 | 1.8 |
| VolumeNormalizer.Update | 1.0 | 0.019 | 0.5 | 18.6 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.018 | 0.5 | 18.1 |
| AtmosphereFromGround.LateUpdate | 5.0 | 0.018 | 0.5 | 3.6 |
| ProgressTracking.Update | 1.0 | 0.017 | 0.4 | 17.1 |
| Part.Update | 21.0 | 0.015 | 0.4 | 2.7 |

### E. Top 20 instrumented methods by self time: run1-S ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.722 | 12.5 | 120.3 |
| NavBall.Update | 1.0 | 0.130 | 2.2 | 130.1 |
| FloatingOrigin.setOffset | 0.3 | 0.102 | 1.8 | 353.5 |
| SpeedDisplay.LateUpdate | 1.0 | 0.102 | 1.8 | 102.2 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.094 | 1.6 | 15.6 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.085 | 1.5 | 14.2 |
| FlightCamera.LateUpdate | 1.0 | 0.067 | 1.1 | 66.5 |
| OrbitRendererBase.LateUpdate | 18.9 | 0.064 | 1.1 | 3.4 |
| ScaledSpace.LateUpdate | 1.0 | 0.055 | 0.9 | 54.7 |
| TemperatureGaugeSystem.Update | 1.0 | 0.053 | 0.9 | 53.4 |
| CommNetNetwork.Update | 1.0 | 0.047 | 0.8 | 54.6 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.045 | 0.8 | 44.8 |
| ModuleEngines.FixedUpdate | 0.9 | 0.038 | 0.7 | 43.6 |
| ScaledSpaceFader.Update | 16.0 | 0.028 | 0.5 | 1.7 |
| Part.ModulesOnUpdate | 21.0 | 0.025 | 0.4 | 1.7 |
| StageGroup.Update | 2.0 | 0.025 | 0.4 | 12.3 |
| ProgressTracking.Update | 1.0 | 0.022 | 0.4 | 22.5 |
| VolumeNormalizer.Update | 1.0 | 0.022 | 0.4 | 22.2 |
| TimeOfDayAnimation.LateUpdate | 1.0 | 0.022 | 0.4 | 22.1 |
| AtmosphereFromGround.LateUpdate | 5.0 | 0.021 | 0.4 | 4.3 |

### E. Top 20 instrumented methods by self time: run2-M2 pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.554 | 10.7 | 92.4 |
| Part.ModulesOnUpdate | 153.0 | 0.118 | 2.3 | 1.0 |
| FI.UpdateAerodynamics(per part) | 39.7 | 0.097 | 1.9 | 2.5 |
| SpeedDisplay.LateUpdate | 1.0 | 0.097 | 1.9 | 97.4 |
| SmokeTrailControl.Update | 68.0 | 0.096 | 1.9 | 1.4 |
| FI.UpdateMassStats | 0.3 | 0.085 | 1.6 | 326.6 |
| Part.Update | 153.0 | 0.078 | 1.5 | 1.6 |
| TemperatureGaugeSystem.Update | 1.0 | 0.072 | 1.4 | 72.2 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.068 | 1.3 | 11.4 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.062 | 1.2 | 10.4 |
| CommNetNetwork.Update | 1.0 | 0.062 | 1.2 | 68.6 |
| VolumeNormalizer.Update | 1.0 | 0.061 | 1.2 | 60.8 |
| Part.FixedUpdate | 39.7 | 0.060 | 1.2 | 1.5 |
| PartBuoyancy.FixedUpdate | 39.7 | 0.058 | 1.1 | 1.5 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.053 | 1.0 | 3.0 |
| FlightCamera.LateUpdate | 1.0 | 0.049 | 0.9 | 49.3 |
| VesselPrecalculate.CalculatePhysicsStats | 0.5 | 0.047 | 0.9 | 90.1 |
| CollisionEnhancer.FixedUpdate | 39.7 | 0.047 | 0.9 | 1.2 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.043 | 0.8 | 42.6 |
| ScaledSpace.LateUpdate | 1.0 | 0.041 | 0.8 | 41.2 |

### E. Top 20 instrumented methods by self time: run2-M2 ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.634 | 7.3 | 105.7 |
| FloatingOrigin.setOffset | 0.4 | 0.522 | 6.0 | 1265.5 |
| FI.UpdateAerodynamics(per part) | 66.7 | 0.175 | 2.0 | 2.6 |
| ModuleEngines.FixedUpdate | 7.4 | 0.140 | 1.6 | 18.9 |
| Part.ModulesOnUpdate | 153.0 | 0.128 | 1.5 | 1.1 |
| NavBall.Update | 1.0 | 0.119 | 1.4 | 119.3 |
| FI.UpdateMassStats | 0.4 | 0.105 | 1.2 | 241.0 |
| PartBuoyancy.FixedUpdate | 66.7 | 0.104 | 1.2 | 1.6 |
| SpeedDisplay.LateUpdate | 1.0 | 0.099 | 1.1 | 98.7 |
| Part.FixedUpdate | 66.7 | 0.095 | 1.1 | 1.7 |
| SmokeTrailControl.Update | 68.0 | 0.094 | 1.1 | 1.4 |
| VesselPrecalculate.CalculatePhysicsStats | 0.9 | 0.084 | 1.0 | 88.9 |
| Part.Update | 153.0 | 0.081 | 0.9 | 1.7 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.079 | 0.9 | 13.2 |
| CollisionEnhancer.FixedUpdate | 66.7 | 0.079 | 0.9 | 1.2 |
| TemperatureGaugeSystem.Update | 1.0 | 0.076 | 0.9 | 75.6 |
| FlightCamera.LateUpdate | 1.0 | 0.074 | 0.8 | 73.6 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.074 | 0.8 | 12.3 |
| FlightInputHandler.FixedUpdate | 0.4 | 0.072 | 0.8 | 165.6 |
| FI.UpdateOcclusion(partial,Update) | 0.2 | 0.071 | 0.8 | 348.9 |

### E. Top 20 instrumented methods by self time: run1-M pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.540 | 11.8 | 90.0 |
| Part.ModulesOnUpdate | 150.0 | 0.116 | 2.5 | 1.0 |
| SmokeTrailControl.Update | 68.0 | 0.096 | 2.1 | 1.4 |
| SpeedDisplay.LateUpdate | 1.0 | 0.093 | 2.0 | 92.8 |
| FI.UpdateAerodynamics(per part) | 34.3 | 0.080 | 1.8 | 2.3 |
| Part.Update | 150.0 | 0.076 | 1.7 | 1.5 |
| TemperatureGaugeSystem.Update | 1.0 | 0.072 | 1.6 | 71.6 |
| FI.UpdateMassStats | 0.2 | 0.070 | 1.5 | 306.0 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.065 | 1.4 | 10.9 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.061 | 1.3 | 10.2 |
| CommNetNetwork.Update | 1.0 | 0.058 | 1.3 | 64.0 |
| VolumeNormalizer.Update | 1.0 | 0.056 | 1.2 | 55.8 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.051 | 1.1 | 2.8 |
| Part.FixedUpdate | 34.3 | 0.051 | 1.1 | 1.5 |
| PartBuoyancy.FixedUpdate | 34.3 | 0.049 | 1.1 | 1.4 |
| FlightCamera.LateUpdate | 1.0 | 0.048 | 1.0 | 47.8 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.043 | 0.9 | 43.0 |
| ScaledSpace.LateUpdate | 1.0 | 0.042 | 0.9 | 42.4 |
| CollisionEnhancer.FixedUpdate | 34.3 | 0.041 | 0.9 | 1.2 |
| VesselPrecalculate.CalculatePhysicsStats | 0.5 | 0.037 | 0.8 | 81.3 |

### E. Top 20 instrumented methods by self time: run1-M ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.647 | 8.0 | 107.9 |
| FloatingOrigin.setOffset | 0.4 | 0.453 | 5.6 | 1222.5 |
| FI.UpdateAerodynamics(per part) | 60.8 | 0.156 | 1.9 | 2.6 |
| ModuleEngines.FixedUpdate | 6.9 | 0.138 | 1.7 | 20.0 |
| TemperatureGaugeSystem.Update | 1.0 | 0.132 | 1.6 | 132.0 |
| Part.ModulesOnUpdate | 150.0 | 0.132 | 1.6 | 1.1 |
| NavBall.Update | 1.0 | 0.119 | 1.5 | 119.0 |
| SpeedDisplay.LateUpdate | 1.0 | 0.094 | 1.2 | 93.6 |
| PartBuoyancy.FixedUpdate | 60.8 | 0.093 | 1.2 | 1.5 |
| SmokeTrailControl.Update | 68.0 | 0.093 | 1.1 | 1.4 |
| FI.UpdateMassStats | 0.4 | 0.089 | 1.1 | 219.0 |
| Part.FixedUpdate | 60.8 | 0.086 | 1.1 | 1.7 |
| Part.Update | 150.0 | 0.082 | 1.0 | 1.7 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.079 | 1.0 | 13.2 |
| CollisionEnhancer.FixedUpdate | 60.8 | 0.075 | 0.9 | 1.2 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.075 | 0.9 | 12.4 |
| VesselPrecalculate.CalculatePhysicsStats | 0.8 | 0.072 | 0.9 | 88.7 |
| FlightCamera.LateUpdate | 1.0 | 0.072 | 0.9 | 71.6 |
| FlightInputHandler.FixedUpdate | 0.4 | 0.066 | 0.8 | 161.8 |
| CommNetNetwork.Update | 1.0 | 0.061 | 0.8 | 67.9 |

### E. Top 20 instrumented methods by self time: run1-L pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| FI.UpdateAerodynamics(per part) | 241.7 | 0.633 | 5.8 | 2.6 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.629 | 5.8 | 104.8 |
| FI.UpdateMassStats | 0.5 | 0.483 | 4.5 | 891.8 |
| PartBuoyancy.FixedUpdate | 241.7 | 0.387 | 3.6 | 1.6 |
| Part.ModulesOnUpdate | 446.0 | 0.380 | 3.5 | 1.1 |
| Part.FixedUpdate | 241.7 | 0.370 | 3.4 | 1.5 |
| VesselPrecalculate.CalculatePhysicsStats | 1.1 | 0.292 | 2.7 | 269.4 |
| SmokeTrailControl.Update | 196.0 | 0.276 | 2.5 | 1.4 |
| CollisionEnhancer.FixedUpdate | 241.7 | 0.261 | 2.4 | 1.1 |
| FI.UpdateOcclusion(partial,Update) | 0.3 | 0.234 | 2.2 | 932.8 |
| Part.Update | 446.0 | 0.229 | 2.1 | 1.6 |
| FI.Integrate(root) | 0.5 | 0.221 | 2.0 | 1576.1 |
| FI.UpdateThermodynamics | 0.5 | 0.175 | 1.6 | 633.9 |
| CommNetNetwork.Update | 1.0 | 0.154 | 1.4 | 162.5 |
| FI.UpdateConduction | 0.5 | 0.150 | 1.4 | 277.1 |
| VolumeNormalizer.Update | 1.0 | 0.145 | 1.3 | 145.4 |
| TemperatureGaugeSystem.Update | 1.0 | 0.128 | 1.2 | 128.5 |
| Part.LateUpdate | 446.0 | 0.120 | 1.1 | 0.3 |
| SpeedDisplay.LateUpdate | 1.0 | 0.103 | 1.0 | 103.5 |
| FI.DragCubeSetupAndPartAeroStats | 0.5 | 0.102 | 0.9 | 187.7 |

### E. Top 20 instrumented methods by self time: run1-L ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| Part.FixedUpdate | 332.4 | 1.710 | 7.7 | 5.5 |
| ModuleEngines.FixedUpdate | 51.2 | 1.196 | 5.4 | 23.3 |
| FI.UpdateAerodynamics(per part) | 332.3 | 0.884 | 4.0 | 2.7 |
| FI.UpdateMassStats | 1.0 | 0.697 | 3.2 | 666.3 |
| PQS.UpdateQuads(coroutine) | 6.0 | 0.633 | 2.9 | 105.4 |
| PartBuoyancy.FixedUpdate | 332.3 | 0.597 | 2.7 | 1.8 |
| VesselPrecalculate.CalculatePhysicsStats | 2.2 | 0.441 | 2.0 | 199.7 |
| TemperatureGaugeSystem.Update | 1.0 | 0.392 | 1.8 | 392.0 |
| FlightInputHandler.FixedUpdate | 1.0 | 0.386 | 1.7 | 369.4 |
| SmokeTrailControl.Update | 242.3 | 0.359 | 1.6 | 1.5 |
| CollisionEnhancer.FixedUpdate | 332.4 | 0.353 | 1.6 | 1.1 |
| FI.Integrate(root) | 1.0 | 0.341 | 1.5 | 1171.3 |
| Part.ModulesOnUpdate | 300.5 | 0.289 | 1.3 | 1.2 |
| FI.UpdateOcclusion(partial,Update) | 0.4 | 0.284 | 1.3 | 659.8 |
| FI.UpdateOcclusion(all,fixed) | 0.1 | 0.248 | 1.1 | 1812.5 |
| VesselDeltaV.FixedUpdate | 1.0 | 0.242 | 1.1 | 231.5 |
| FI.UpdateThermodynamics | 1.0 | 0.230 | 1.0 | 439.5 |
| FI.CheckThermalGraph | 1.0 | 0.223 | 1.0 | 213.5 |
| FI.UpdateConduction | 1.0 | 0.206 | 0.9 | 196.7 |
| FI.DragCubeSetupAndPartAeroStats | 1.0 | 0.202 | 0.9 | 192.9 |

### F. PartModule work per physics tick (ascent; inclusive ms/tick, calls/tick)

| module method | S ms/tick (calls/tick) | M2 ms/tick (calls/tick) | M ms/tick (calls/tick) | L ms/tick (calls/tick) |
|---|---|---|---|---|
| ModuleEngines.FixedUpdate | 0.1308 (3.0) | 0.3218 (17.0) | 0.3410 (17.0) | 1.1432 (49.0) |
| FXModuleAnimateThrottle.FixedUpdate | 0.0095 (2.0) | 0.0279 (16.0) | 0.0274 (16.0) | 0.0791 (48.0) |
| ModuleGimbal.FixedUpdate | 0.0070 (3.0) | 0.0234 (17.0) | 0.0221 (17.0) | 0.0722 (49.0) |
| ModuleAlternator.FixedUpdate | 0.0060 (3.0) | 0.0159 (17.0) | 0.0156 (17.0) | 0.0549 (49.0) |
| ModuleCommand.FixedUpdate | 0.0130 (1.0) | 0.0112 (1.0) | 0.0111 (1.0) | 0.0315 (1.0) |
| ModuleReactionWheel.FixedUpdate | 0.0043 (1.0) | 0.0032 (1.0) | 0.0031 (1.0) | 0.0037 (1.0) |
| ModuleCargoPart.OnFixedUpdate | 0.0003 (2.0) | 0.0014 (16.0) | 0.0012 (16.0) | 0.0039 (48.0) |
| FXModuleAnimateThrottle.OnFixedUpdate | 0.0003 (3.0) | 0.0014 (17.0) | 0.0012 (17.0) | 0.0031 (49.0) |
| ModuleJettison.OnFixedUpdate | 0.0003 (3.0) | 0.0010 (17.0) | 0.0008 (17.0) | 0.0025 (49.0) |
| ModuleAlternator.OnFixedUpdate | 0.0003 (3.0) | 0.0010 (17.0) | 0.0009 (17.0) | 0.0025 (49.0) |
| ModuleGimbal.OnFixedUpdate | 0.0003 (3.0) | 0.0010 (17.0) | 0.0008 (17.0) | 0.0023 (49.0) |
| ModuleSurfaceFX.OnFixedUpdate | 0.0003 (3.0) | 0.0009 (17.0) | 0.0009 (17.0) | 0.0022 (49.0) |
| ModuleTestSubject.OnFixedUpdate | 0.0003 (3.0) | 0.0008 (17.0) | 0.0012 (17.0) | 0.0021 (49.0) |
| ModuleEngines.OnFixedUpdate | 0.0003 (3.0) | 0.0008 (17.0) | 0.0007 (17.0) | 0.0020 (49.0) |
| ModuleSAS.FixedUpdate | 0.0003 (1.0) | 0.0003 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) |
| ModulePartVariants.OnFixedUpdate | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) | 0.0002 (1.0) |
| FXModuleLookAtConstraint.OnFixedUpdate | 0.0002 (1.0) | 0.0002 (1.0) | 0.0001 (1.0) | 0.0002 (1.0) |
| FXModuleLookAtConstraint.FixedUpdate | 0.0000 (1.0) | 0.0001 (1.0) | 0.0000 (1.0) | 0.0000 (1.0) |

### G. Instrumentation overhead estimate

| run | instrumented calls/frame | pair cost ns | est. overhead ms/frame | % of frame |
|---|---|---|---|---|
| run1-S pad | 631 | 50.7 | 0.032 | 0.8% |
| run1-S ascent | 660 | 50.7 | 0.033 | 0.6% |
| run2-M2 pad | 1692 | 49.2 | 0.083 | 1.6% |
| run2-M2 ascent | 1905 | 49.2 | 0.094 | 1.1% |
| run1-M pad | 1659 | 50.0 | 0.083 | 1.8% |
| run1-M ascent | 1864 | 50.0 | 0.093 | 1.2% |
| run1-L pad | 4674 | 49.1 | 0.229 | 2.1% |
| run1-L ascent | 4927 | 49.1 | 0.242 | 1.1% |

### H. PhysX and FixedUpdate-script cost per tick, split by whether the tick had a floating-origin shift (ascent windows)

| run | ticks without shift | PhysX ms/tick | scripts ms/tick | ticks with shift | PhysX ms/tick | scripts ms/tick |
|---|---|---|---|---|---|---|
| run1-S | (no fo_shifts column in this build) | | | | | |
| run2-M2 | (no fo_shifts column in this build) | | | | | |
| run1-M | (no fo_shifts column in this build) | | | | | |
| run1-L | (no fo_shifts column in this build) | | | | | |

### I. Frames containing a GC collection

| run | window | median frame ms | GC frames (frame ms) | managed heap MB (min-max) |
|---|---|---|---|---|
| run1-S | pad | 3.60 | none | 648-676 |
| run1-S | ascent | 4.85 | 17 | 650-688 |
| run2-M2 | pad | 4.30 | 13 | 666-693 |
| run2-M2 | ascent | 5.92 | 19 | 672-715 |
| run1-M | pad | 3.87 | none | 666-688 |
| run1-M | ascent | 5.58 | 21 | 672-710 |
| run1-L | pad | 12.95 | 23 | 720-748 |
| run1-L | ascent | 19.04 | 67, 92, 44, 45, 51, 45, 39 | 742-1019 |

