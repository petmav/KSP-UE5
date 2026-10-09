### A. Headline numbers

| craft | window | parts | rigidbodies | attach joints (CJ) | all joints | colliders | FPS | frame ms mean | p50 | p95 | p99 | fixed/frame | sim rate (UT/wall) | GC/min | alloc MB/s (lower bound) | alt end m | srf speed end |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| run4-M2 | pad | 153 | 153 | 168 | 169 | 154 | 194.9 | 5.13 | 4.20 | 8.51 | 11.94 | 0.257 | 1.000 | 0.0 | 1.01 | 102 | 0 |
| run4-M2 | ascent | 153 | 153 | 168 | 168 | 154 | 115.2 | 8.68 | 5.98 | 15.87 | 19.73 | 0.434 | 1.000 | 2.0 | 0.81 | 6068 | 384 |
| run5-M2-gfxdefault | pad | 153 | 153 | 168 | 169 | 154 | 140.7 | 7.11 | 6.33 | 10.48 | 12.58 | 0.355 | 1.000 | 0.0 | 0.79 | 102 | 0 |
| run5-M2-gfxdefault | ascent | 153 | 153 | 168 | 168 | 154 | 85.7 | 11.66 | 13.56 | 17.04 | 20.05 | 0.583 | 1.000 | 2.0 | 0.65 | 6068 | 384 |

### B. Frame breakdown by Unity player-loop phase (ms per rendered frame, % of frame)

| phase | M2 pad | M2 ascent | M2-gfxdefault pad | M2-gfxdefault ascent |
|---|---|---|---|---|
| FixedUpdate scripts | 0.604 (12%) | 1.847 (21%) | 0.996 (14%) | 2.729 (23%) |
| Rendering (cameras, renderers, present) | 0.989 (19%) | 1.653 (19%) | 1.556 (22%) | 2.727 (23%) |
| Update scripts | 1.234 (24%) | 1.523 (18%) | 1.568 (22%) | 1.894 (16%) |
| PhysX step (+collision callbacks) | 0.205 (4%) | 0.950 (11%) | 0.336 (5%) | 1.301 (11%) |
| Coroutines (incl. PQS terrain) | 0.820 (16%) | 0.940 (11%) | 1.059 (15%) | 1.040 (9%) |
| LateUpdate scripts | 0.692 (13%) | 0.846 (10%) | 0.854 (12%) | 0.900 (8%) |
| UI canvases (uGUI) | 0.231 (4%) | 0.381 (4%) | 0.299 (4%) | 0.419 (4%) |
| Other engine | 0.146 (3%) | 0.169 (2%) | 0.182 (3%) | 0.219 (2%) |
| Particles | 0.004 (0%) | 0.105 (1%) | 0.005 (0%) | 0.123 (1%) |
| Audio | 0.082 (2%) | 0.107 (1%) | 0.104 (1%) | 0.111 (1%) |
| Animation | 0.093 (2%) | 0.094 (1%) | 0.106 (1%) | 0.100 (1%) |
| FixedUpdate other | 0.005 (0%) | 0.035 (0%) | 0.009 (0%) | 0.064 (1%) |
| Outside player loop (frame wait / present) | 0.027 (1%) | 0.029 (0%) | 0.036 (1%) | 0.038 (0%) |
| **total** | 5.132 | 8.680 | 7.109 | 11.666 |

### C. Physics-tick components (ms per physics tick; inclusive)

| component | M2 pad | M2 ascent | M2-gfxdefault pad | M2-gfxdefault ascent |
|---|---|---|---|---|
| Planetarium.FixedUpdate (UT, body orbits) | 0.071 | 0.077 | 0.096 | 0.098 |
| VesselPrecalculate.FixedUpdate | 0.017 | 0.023 | 0.027 | 0.031 |
| OrbitDriver.FixedUpdate (all drivers) | 0.000 | 0.001 | 0.001 | 0.001 |
| CollisionEnhancer.FixedUpdate (per part) | 0.184 | 0.187 | 0.241 | 0.216 |
| FlightInputHandler.FixedUpdate (input, SAS/autopilot) | 0.049 | 0.173 | 0.068 | 0.197 |
| Part.FixedUpdate (per part, incl. OnFixedUpdate) | 0.251 | 0.271 | 0.343 | 0.332 |
| PartModule Unity FixedUpdates (sum) | 0.060 | 0.416 | 0.078 | 0.496 |
| PartBuoyancy.FixedUpdate (per part) | 0.242 | 0.248 | 0.271 | 0.276 |
| FlightIntegrator.FixedUpdate (total) | 1.207 | 1.167 | 1.330 | 1.250 |
|   FI.UpdateMassStats | 0.353 | 0.255 | 0.413 | 0.292 |
|   FI.DragCubeSetupAndPartAeroStats | 0.061 | 0.057 | 0.068 | 0.058 |
|   FI.CheckThermalGraph | 0.001 | 0.001 | 0.001 | 0.001 |
|   FI.UpdateThermodynamics | 0.242 | 0.269 | 0.261 | 0.289 |
|     of which UpdateConduction | 0.109 | 0.132 | 0.116 | 0.141 |
|   FI.Integrate (force application, self) | 0.143 | 0.155 | 0.160 | 0.164 |
|   FI.UpdateAerodynamics (drag, body lift) | 0.397 | 0.418 | 0.414 | 0.433 |
| Vessel.FixedUpdate | 0.001 | 0.002 | 0.001 | 0.002 |
| Krakensbane.FixedUpdate | 0.003 | 0.034 | 0.003 | 0.036 |
| FloatingOrigin.FixedUpdate (incl. setOffset) | 0.001 | 1.240 | 0.001 | 1.274 |
| CollisionManager.FixedUpdate | 0.000 | 0.002 | 0.000 | 0.002 |
| PQS.FixedUpdate | 0.010 | 0.012 | 0.017 | 0.017 |
| **All FixedUpdate scripts (measured slot)** | 2.353 | 4.256 | 2.805 | 4.682 |
| **PhysX simulate (PhysicsFixedUpdate slot minus callbacks)** | 0.798 | 2.190 | 0.945 | 2.231 |
| OnCollision*/OnJointBreak callbacks (inside PhysX slot) | 0.002 | 0.000 | 0.002 | 0.000 |
| **Physics tick total (scripts + PhysX)** | 3.153 | 6.446 | 3.751 | 6.912 |

### D. Per-frame (Update/LateUpdate/coroutine) physics-adjacent work (ms per rendered frame)

| component | M2 pad | M2 ascent | M2-gfxdefault pad | M2-gfxdefault ascent |
|---|---|---|---|---|
| VesselPrecalculate.Update (CoM/MOI stats, orbit) | 0.057 | 0.103 | 0.095 | 0.146 |
|   of which CalculatePhysicsStats (all calls) | 0.047 | 0.085 | 0.076 | 0.119 |
| FlightIntegrator.Update (sun raycast, occlusion) | 0.091 | 0.131 | 0.125 | 0.171 |
| Part.Update (incl. OnUpdate) | 0.255 | 0.266 | 0.307 | 0.307 |
| PQS.UpdateQuads (coroutine) | 0.605 | 0.687 | 0.783 | 0.763 |
| PQS.UpdateVisual + Mod hooks (coroutine) | 0.147 | 0.175 | 0.189 | 0.189 |
| KSP/UI/* scripts (self time, all callbacks) | 0.298 | 0.421 | 0.357 | 0.475 |

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

### E. Top 20 instrumented methods by self time: run5-M2-gfxdefault pad

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.783 | 11.0 | 130.5 |
| Part.ModulesOnUpdate | 153.0 | 0.157 | 2.2 | 1.3 |
| FI.UpdateAerodynamics(per part) | 54.4 | 0.147 | 2.1 | 2.7 |
| FI.UpdateMassStats | 0.4 | 0.147 | 2.1 | 412.4 |
| Part.FixedUpdate | 54.4 | 0.122 | 1.7 | 2.2 |
| SpeedDisplay.LateUpdate | 1.0 | 0.116 | 1.6 | 115.9 |
| SmokeTrailControl.Update | 68.0 | 0.116 | 1.6 | 1.7 |
| PartBuoyancy.FixedUpdate | 54.4 | 0.096 | 1.4 | 1.8 |
| Part.Update | 153.0 | 0.096 | 1.3 | 2.0 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.094 | 1.3 | 15.7 |
| TemperatureGaugeSystem.Update | 1.0 | 0.091 | 1.3 | 90.7 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.090 | 1.3 | 15.0 |
| CollisionEnhancer.FixedUpdate | 54.4 | 0.085 | 1.2 | 1.6 |
| CommNetNetwork.Update | 1.0 | 0.083 | 1.2 | 91.5 |
| VolumeNormalizer.Update | 1.0 | 0.077 | 1.1 | 76.9 |
| VesselPrecalculate.CalculatePhysicsStats | 0.7 | 0.076 | 1.1 | 107.0 |
| FlightCamera.LateUpdate | 1.0 | 0.076 | 1.1 | 75.5 |
| OrbitRendererBase.LateUpdate | 18.0 | 0.069 | 1.0 | 3.8 |
| FI.ThermoPrecalculate(Update) | 1.0 | 0.066 | 0.9 | 66.2 |
| FI.UpdateOcclusion(partial,Update) | 0.2 | 0.058 | 0.8 | 345.8 |

### E. Top 20 instrumented methods by self time: run5-M2-gfxdefault ascent

| method | calls/frame | self ms/frame | self % of frame | incl 탎/call |
|---|---|---|---|---|
| PQS.UpdateQuads(coroutine) | 6.0 | 0.763 | 6.5 | 127.2 |
| FloatingOrigin.setOffset | 0.6 | 0.741 | 6.4 | 1344.4 |
| FI.UpdateAerodynamics(per part) | 89.2 | 0.252 | 2.2 | 2.8 |
| ModuleEngines.FixedUpdate | 9.9 | 0.231 | 2.0 | 23.3 |
| FI.UpdateMassStats | 0.6 | 0.170 | 1.5 | 291.6 |
| Part.FixedUpdate | 89.2 | 0.166 | 1.4 | 2.2 |
| PartBuoyancy.FixedUpdate | 89.2 | 0.161 | 1.4 | 1.8 |
| Part.ModulesOnUpdate | 153.0 | 0.161 | 1.4 | 1.3 |
| NavBall.Update | 1.0 | 0.144 | 1.2 | 143.7 |
| CollisionEnhancer.FixedUpdate | 89.2 | 0.126 | 1.1 | 1.4 |
| VesselPrecalculate.CalculatePhysicsStats | 1.6 | 0.119 | 1.0 | 72.2 |
| SpeedDisplay.LateUpdate | 1.0 | 0.118 | 1.0 | 117.6 |
| SmokeTrailControl.Update | 68.0 | 0.117 | 1.0 | 1.7 |
| FlightInputHandler.FixedUpdate | 0.6 | 0.115 | 1.0 | 197.4 |
| FI.UpdateOcclusion(partial,Update) | 0.3 | 0.097 | 0.8 | 359.8 |
| FI.Integrate(root) | 0.6 | 0.096 | 0.8 | 596.9 |
| PQS.Mod_OnUpdateFinished(coroutine) | 6.0 | 0.096 | 0.8 | 16.0 |
| Part.Update | 153.0 | 0.093 | 0.8 | 2.0 |
| PQS.UpdateVisual(coroutine) | 6.0 | 0.089 | 0.8 | 14.8 |
| TemperatureGaugeSystem.Update | 1.0 | 0.087 | 0.7 | 87.3 |

### F. PartModule work per physics tick (ascent; inclusive ms/tick, calls/tick)

| module method | M2 ms/tick (calls/tick) | M2-gfxdefault ms/tick (calls/tick) |
|---|---|---|
| ModuleEngines.FixedUpdate | 0.3321 (17.0) | 0.3966 (17.0) |
| FXModuleAnimateThrottle.FixedUpdate | 0.0292 (16.0) | 0.0330 (16.0) |
| ModuleGimbal.FixedUpdate | 0.0239 (17.0) | 0.0278 (17.0) |
| ModuleAlternator.FixedUpdate | 0.0161 (17.0) | 0.0194 (17.0) |
| ModuleCommand.FixedUpdate | 0.0116 (1.0) | 0.0146 (1.0) |
| ModuleReactionWheel.FixedUpdate | 0.0032 (1.0) | 0.0042 (1.0) |
| ModuleCargoPart.OnFixedUpdate | 0.0013 (16.0) | 0.0018 (16.0) |
| FXModuleAnimateThrottle.OnFixedUpdate | 0.0013 (17.0) | 0.0015 (17.0) |
| ModuleSurfaceFX.OnFixedUpdate | 0.0011 (17.0) | 0.0010 (17.0) |
| ModuleAlternator.OnFixedUpdate | 0.0009 (17.0) | 0.0011 (17.0) |
| ModuleJettison.OnFixedUpdate | 0.0009 (17.0) | 0.0010 (17.0) |
| ModuleGimbal.OnFixedUpdate | 0.0009 (17.0) | 0.0010 (17.0) |
| ModuleTestSubject.OnFixedUpdate | 0.0009 (17.0) | 0.0010 (17.0) |
| ModuleEngines.OnFixedUpdate | 0.0008 (17.0) | 0.0009 (17.0) |
| ModuleSAS.FixedUpdate | 0.0003 (1.0) | 0.0004 (1.0) |
| ModulePartVariants.OnFixedUpdate | 0.0002 (1.0) | 0.0003 (1.0) |
| FXModuleLookAtConstraint.OnFixedUpdate | 0.0002 (1.0) | 0.0002 (1.0) |
| FXModuleLookAtConstraint.FixedUpdate | 0.0000 (1.0) | 0.0001 (1.0) |

### G. Instrumentation overhead estimate

| run | instrumented calls/frame | pair cost ns | est. overhead ms/frame | % of frame |
|---|---|---|---|---|
| run4-M2 pad | 1697 | 49.3 | 0.084 | 1.6% |
| run4-M2 ascent | 1919 | 49.3 | 0.095 | 1.1% |
| run5-M2-gfxdefault pad | 1783 | 49.2 | 0.088 | 1.2% |
| run5-M2-gfxdefault ascent | 2069 | 49.2 | 0.102 | 0.9% |

### H. PhysX and FixedUpdate-script cost per tick, split by whether the tick had a floating-origin shift (ascent windows)

| run | ticks without shift | PhysX ms/tick | scripts ms/tick | ticks with shift | PhysX ms/tick | scripts ms/tick |
|---|---|---|---|---|---|---|
| run4-M2 | 81 | 0.71 | 3.05 | 1419 | 2.27 | 4.33 |
| run5-M2-gfxdefault | 82 | 0.78 | 3.89 | 1418 | 2.31 | 4.73 |

### I. Frames containing a GC collection

| run | window | median frame ms | GC frames (frame ms) | managed heap MB (min-max) |
|---|---|---|---|---|
| run4-M2 | pad | 4.20 | none | 666-687 |
| run4-M2 | ascent | 5.98 | 20 | 672-711 |
| run5-M2-gfxdefault | pad | 6.33 | none | 683-700 |
| run5-M2-gfxdefault | ascent | 13.56 | 19 | 676-710 |

