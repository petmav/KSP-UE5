# Wheel / landing-leg spike: what WP-29 should build

Status: 2026-10-13, branch `wheelspike` (from main `b9119103`). Spike code: `core/spikes/wheels/` (opt-in target `wheel_spike`, built only
with `KSPCORE_BUILD_SPIKES=ON`, never part of `run_tests.ps1` / ctest / CI). Raw results: `core/spikes/wheels/results/*.md|csv|txt`.
Progress log: `core/PROGRESS-wheelspike.md`. Inputs: [PHYSICS-SPIKE.md](PHYSICS-SPIKE.md) (the reduced rigid-body model the user adopted),
[PHYSICS-ENGINE-REVIEW.md](../analysis/physics/PHYSICS-ENGINE-REVIEW.md) section 7. C# citations are relative to
`decompiled/KSP-Decompiled/Assembly-CSharp/`.

## Summary

**Recommendation: WP-29 ports KSP's wheel code as KSP runs it, and not as a Jolt vehicle.**

- Port the managed part 1:1 and gate it bit-exactly. That is the VPP tyre and wheel-spin solver plus KSP's wheel modules: 8,867 reachable
  lines in 34 files, not the 29,217 lines in WP-29's file list.
- Under it, write a small native WheelCollider model in kspcore (about 1.5-2.5k new lines): one ray per wheel against the Jolt scene,
  PhysX 4.1's suspension law with Unity's `targetPosition` mapping and KSP's `Physics.gravity = 0`, and a hard stop at full compression.
  The managed code receives the WheelHit one tick late, as it does in Unity.
- Wheels and legs are force elements on the vessel's rigid cluster, and the wheel parts are welded into that cluster. KSP itself
  force-autostruts every wheel to the heaviest part (ModuleWheelBase.cs:220).
- Physics warp runs as 0.02 s substeps for any vessel with grounded wheels. A larger dt makes both the native suspension and the managed
  VPP brake/tyre model misbehave.
- Do not use Jolt's `WheeledVehicleController`: different tyre and drivetrain model, a behaviour change. Keep Jolt's `VehicleConstraint`
  (suspension only, null friction controller) as the measured fallback if large-dt warp is ever wanted.

What the spike measured on this machine (Ryzen 9 7950X3D, one thread pinned, MSVC Release, Jolt v5.6.0 double precision). The craft are
three real stock craft loaded through WP-18 `ShipConstruct` and WP-14 assembly: Bug-E Buggy (8 x RoveMax S2), Prospector Rover
(4 x RoveMax M1) and Two-Stage Lander (4 x LT-2), on Kerbin's 900-quad PQS patch or a flat plane.

| | explicit PhysX law (recommended) | Jolt VehicleConstraint (fallback) | reference |
|---|---|---|---|
| pad rest, 60 s, 1x, braked: CoM drift | 0.3 / 0.5 / 0.5 mm (Bug-E / Prospector / lander) | 0.9 / 1.1 / 2.2 mm | -- |
| pad rest, unbraked, 1x | 5.8 / 7.2 mm creep | 6.5 / 7.3 mm | physics: centrifugal north component at KSC's latitude -0.097 deg, 8.6e-5 m/s^2 (an unbraked KSP rover creeps the same way) |
| pad rest at 4x physics warp (dt 0.08) | **unstable**: Bug-E lifts off, Prospector drifts 100 m; lander already unstable at 3x (18.7 m) | suspension stable (Prospector 0.8 mm, lander 5.5 mm); braked Bug-E creeps 7.8 m (VPP tyre limit cycle, from 2x: 1.0 m) | -- |
| drive, full throttle, 1x, flat: speed at 10 / 20 / 30 / 40 s | Prospector 19.41 / 28.36 / 33.36 / 37.01 m/s; Bug-E 7.95 / 9.89 / 10.71 / 10.71 | same within 0.1 | C# motor model without tyre slip + VPP wheel inertia: 19.41 / 28.35 / 33.40 / 37.15; Bug-E 7.36 / 9.70 / 10.55 / 10.71 |
| drive at 4x | Prospector -16 % at 10 s (suspension) | Prospector within 0.4 % of its 4x reference | -- |
| brake from 8 m/s | Bug-E 8.30 m, Prospector 7.34 m | 8.20 m, 7.39 m | KSP brake torque, tyre-curve capped: 8.04 m, 6.05 m |
| landing legs, 3 m/s touchdown, Kerbin | 50 % of travel (0.60 m), rebound 1.87 m/s, no leg leaves the ground, settles 1.4 s | 62 % (0.73 m), 1.71 m/s, body touches | at rest: rest offset 0.12 m + W / k = 27.5 kN / 158 kN/m = 0.17 m, so 0.29 m; measured 0.29 m |
| cost per wheel per tick | 0.60-0.87 us (managed 0.28-0.44 with 8 VPP substeps, native ray + law 0.32-0.44) | managed 0.28-0.46 + ~0.7 us inside the Jolt step | KSP: no wheeled craft was ever profiled |
| whole rover tick (4-10 wheels) | 6-13 us | 7-16 us | -- |

Surprises found before building (each is cited and explained in section 1.5):

1. **The tyre curve in effect is probably `SmoothFriction`, not the `Lineal` KSP asks for.** `ModuleWheelBase.cs:361` writes
   `tireFriction.model = Lineal`, but the curve object is only rebuilt by `SetupFrictionCurves()` in the constructor and in
   `OnAfterDeserialize`. Measured effect: about 3 % on acceleration when the slip is in the 0.5-3 m/s band (Mun drive), none elsewhere.
   A capture settles it.
2. **About 70 % of WP-29 is dead code.** Edy's whole Vehicle Physics Pro library ships in the assembly (VPVehicleController, Engine,
   Gearbox, Driveline, audio, cameras, telemetry, charts, cargo, UDP, JSON, TextureCanvas ...). Nothing in KSP references it. The wheel
   runtime reaches 8,867 of 29,217 lines.
3. **The native part is small and pinned down.** With KSP's `Physics.gravity = 0` [runtime], PhysX's suspension force reduces to
   `max(0, k (c - targetPosition D)) + d dc/dt` along the hit normal. VPP zeroes PhysX's tyre friction, and KSP computes `k`, `d` and
   `targetPosition` in managed code every tick.
4. **KSP's auto-tuned spring is proportional to the per-wheel load in g-tonnes**, so the spring deflection,
   g0 / (springRatio x springTweakable x boost), does not depend on mass or gravity. That is 0.3 m before tweak and boost.
   - On top of it comes the PhysX rest offset of 10-17 % of the travel.
   - `boost` and `autoBoost` (when compression passes 80 %) stiffen the spring.
   - Wheels sit at 25-53 % of their travel.
   - On the Mun the same impact speed meets softer springs and compresses further.
5. **VPP floors wheel inertia at 0.01 t m^2.** That is 20x the real value of a RoveMax S2. Each S2 wheel adds 0.41 t of effective mass,
   which makes the 1.29 t Bug-E accelerate like a 4.5 t vehicle. The port must keep this.
6. **The M1 wheel collider is 1.25x larger than the radius the motor and traction control use.** The default `rescaleFactor` applies to
   the collider only, so traction control cuts torque by 0.4 % per m/s at 1x and 4x as much at 4x warp.
7. **Physics warp is not safe for wheels at dt = 0.02 x rate**, and that comes from KSP's own managed code too, not only from the solver.
8. **Wheel / leg geometry needs the `.mu` models.** The two available proxies place the LT-2 foot 0.85 m apart, and that decides whether
   the lander touches down on its legs or on its engines.

## Contents

1. [What the C# does](#1-what-the-c-does)
2. [Jolt implementation strategy](#2-jolt-implementation-strategy)
3. [The prototype](#3-the-prototype)
4. [Measurements](#4-measurements)
5. [Comparison with KSP](#5-comparison-with-ksp)
6. [Risks](#6-risks)
7. [WP-29 package split and gates](#7-wp-29-package-split-and-gates)
8. [Reproducing](#8-reproducing)

## 1. What the C# does

### 1.1 Structure

- **One vehicle per wheel part.** `ModuleWheelBase.wheelSetup` (ModuleWheelBase.cs:277-374) adds a `KSPWheelController : VehicleBase`
  (KSPWheelController.cs:174-228) with ONE VPP `Wheel` block driven by a `DirectDrive`, to the wheel part's own Rigidbody. A rover
  with 8 wheels runs 8 independent VPP vehicles. They are coupled only through the rigid bodies and joints.
- **`VPWheelCollider` wraps a real `UnityEngine.WheelCollider`** (VPWheelCollider.cs:397-412). Its PhysX tyre friction stiffness is 0,
  its wheel mass is 1, it gets `motorTorque = 1e-5` (:416, keeps PhysX awake) and one vehicle substep (`ConfigureVehicleSubsteps(1000, 1, 1)`,
  :197). PhysX therefore contributes only the suspension ray, the suspension force and the `WheelHit`.
- **Landing legs are wheels.** They use `wheelType = LEG` with `ModuleWheelSuspension`, `ModuleWheelDeployment`, `ModuleWheelLock`,
  `ModuleWheelBogey` and `ModuleWheelDamage`. `ModuleLandingLeg` exists only in a save upgrader.
- **Wheels hold a parked vessel by themselves.** A vessel whose ground contacts carry `ModuleWheelBase` never gets the landed "RB anchor"
  (Vessel.cs:1105-1166, exception list :1565-1569).
- **Wheel parts are force-autostrutted to the heaviest part** (`autoStrutMode = ForceHeaviest`, ModuleWheelBase.cs:220). So KSP already
  keeps them nearly rigid with the vessel.

### 1.2 Native vs managed

| piece | where | kind | what WP-29 must do |
|---|---|---|---|
| suspension ray along the travel (layer mask = DefaultEquivalent - layer 17 + layer 15, ModuleWheelBase.cs:300-304; own colliders ignored). The spike casts from one radius above the top-of-travel wheel centre over travel + 2 radii, so a contact deeper than the travel still registers for the hard stop | `UnityEngine.WheelCollider` -> PhysX `PxVehicleSuspensionRaycasts` | **native** | model: a Jolt ray / shape cast per wheel against static + other-vessel layers |
| suspension force `F = max(0, sprungMass * max(0, g.w) + k * jounce) + c * jounceSpeed`, magnitude `hitNorm.(-w F)` along the hit normal, `jounce = min(dx, maxCompression)`, `jounceSpeed = (jounce - prevJounce) / dt` | PhysX 4.1 `PxVehicleUpdate.cpp` `processSuspTireWheels` | **native**, open source | model the formula. KSP's `Physics.gravity = (0,0,0)` [runtime, PROFILE.md 6] makes the sprung-mass term 0 |
| rest pose: `targetPosition` 0 = full extension, 1 = full compression (Unity docs), so the rest compression is `c0 = targetPosition * D`, maxDroop = c0, maxCompression = D - c0 | Unity glue | **native** (documented, not verified in game) | model; capture to confirm |
| hard stop at full compression (`suspLimitActiveFlags`: a constraint, not in `WheelHit.force`) | PhysX | **native** | model: a velocity constraint (PGS rows) or Jolt's `mSuspensionMaxUpPart` |
| `WheelCollider.sprungMass` (Unity distributes the rigidbody mass over its wheel colliders: one collider per wheel part, so it is that part's rb.mass) | Unity glue | **native** | model; read by VPWheelCollider.cs:438 for the spring floor and `targetPosition` |
| application point `forceAppPointDistance` (KSP sets it every tick so the force acts at the contact point, VPWheelCollider.cs:304-311 via VehicleBase.cs:740-743) | Unity / PhysX | **native** | model: apply at the contact point |
| `WheelHit` (point, normal, force = `suspSpringForce`, collider, forwardSlip / sidewaysSlip) from the LAST `Physics.Simulate` | Unity / PhysX | **native** | model; VPP adds `rb.velocity * fixedDeltaTime` to the point (VPWheelCollider.cs:324-327). `sidewaysSlip` (PhysX `atan(latSpeed / (|longSpeed| + 1))`) is read only by ModuleWheelSteering.cs:283 |
| `Rigidbody`: `AddForceAtPosition`, `GetPointVelocity`, `velocity`, `maxAngularVelocity = 14` on the wheel part (VehicleBase.cs:906), `maxDepenetrationVelocity` lerp (:936-944), `RigidbodyConstraints.FreezeAll` "schloompa" (ModuleWheelBase.cs:602-621, :785-799) | Unity / PhysX | native API | WP-20b `VesselPhysics` calls; the freeze becomes a 2 s vessel anchor |
| visual ray per frame (VPWheelCollider.cs:241-302) | Unity | native | the renderer's business (suspension transform position) |
| everything else: `VehicleBase.FixedUpdate` (VehicleBase.cs:934-1033), `Wheel` / `DirectDrive` / `Block` / `Solver` (explicit Euler, 8 substeps active / 4 inactive, GameSettings.cs:1061-1062), `TireFriction`, `VPWheelCollider.UpdateWheelCollider` / `GetGroundHit` fix-ups / `GetContactDepth`, `KSPWheelController`, `KSPWheelGravity`, `SuspensionLoadBalancer`, `ModuleWheelBase` and the 13 `ModuleWheels/*` files | Assembly-CSharp | **managed** | port 1:1, gate bit-exactly (Mono float rule) |

### 1.3 The managed tick (per wheel, per FixedUpdate)

The ported version is `core/spikes/wheels/src/wheel_model.cpp`. Every statement there cites the C# line.

1. `SuspensionLoadBalancer.FixedUpdate` (SuspensionLoadBalancer.cs:31-59), a VesselModule. It sets `sprungMassGravity` to the vessel
   mass times `round(100 / grounded) / 100`, at least 0.5. When Landed, the mass is first scaled by `gravityMultiplier x GeeASL` and
   rounded to 0.1. With 8 grounded wheels each gets 0.12, so 96 % of the mass in total.
2. `ModuleWheelBase.FixedUpdate` (:421-463). It computes the slip displacement with the **unscaled** cfg radius, refreshes the gravity,
   sets `maxRpm` from `wheelMaxSpeed`, and runs landing detection.
3. `ModuleWheelSuspension.SuspensionSpringUpdate` (ModuleWheelSuspension.cs:355-419), only while grounded:
   - `spring = clamp(springTweakable springRatio sprungMassGravity BoostCurve(boost, boostRatio + autoBoost), 0.01, 5000..10000)`.
   - `damper = sqrt(clamp(spring damperRatio fudge)) damperTweakable`.
   - High-gee multipliers apply above 1.5 g.
   - Both are rounded to integers, and hysteresis is 3 kN/m and 1.05.
   - `autoBoost` grows by 0.2 per s while compression > 0.8 (:421-435).
4. `ModuleWheelMotor.FixedUpdate` (ModuleWheelMotor.cs:131-176):
   - `maxDriveTorque = round(100 torqueCurve(|w r_cfg|)) / 100`.
   - Traction control multiplies the input by `1 - |slipDisplacement.y tractionControlScale|`.
   - The drive limiter is applied, then electric charge is drawn.
5. `ModuleWheelBrakes.FixedUpdate` (ModuleWheelBrakes.cs:88-104): `brakeInput * brakeTweakable% * min(1, lerp(1, GeeASL, |w|))`.
   `ModuleWheelLock` (legs, ModuleWheelLock.cs:8-13) sets a permanent brake input of 1 with response 1000.
6. `KSPWheelController` / `VehicleBase.FixedUpdate`:
   - Read the last WheelHit. Contact depth = compression from full extension (VPWheelCollider.cs:351-359).
   - `downforce = hit.force cos(contactAngle)`, at least `minimumDownforce`: 0.5 kN for wheels, **10 kN for legs** (ModuleWheelBase.cs:271-274).
   - Local slip velocities. External tyre force `-0.5 (hit.force / g) v / dt` + a slope term at walking speed.
   - Drive and brake ramps (`MoveTowards` at `driveResponse` / `brakeResponse`). Once `brakeState >= 1` the brake torque becomes
     `maxBrakeTorqueRest = 1000` kN m (KSPWheelController.cs:256-263).
   - `Solver.Integrate`: 8 explicit Euler substeps of the Wheel / DirectDrive pair, the tyre force from the friction curve with an
     adherent (linear) region.
   - Smooth the tyre force (`lerp(last, new, 10 dt)`) and apply it at `hit.point` (VehicleBase.cs:827-849).
7. `VPWheelCollider.UpdateWheelCollider` (:414-473) writes the native spring for the coming step:
   - `spring = max(runtimeSpring, sprungMass g / D)`, `damper = max(runtimeDamper, 0.1)`.
   - `targetPosition = clamp(sprungMass g / spring / D, 0.1, 1)`, changed by at most 0.02 per tick.
   - `sprungMass` is the wheel part's rb mass (0.04-0.1 t), so `targetPosition` sits at or near its 0.1 floor: S2 and LT-2 0.10,
     M1 0.17 measured. The rest offset `c0` is therefore 10-17 % of the travel.
   - Unity's default `targetPosition` is 0.5, and the target moves 0.02 per tick, so the spawn transient lasts about 0.4 s.

### 1.4 Order inside a tick

The order assumed here is the documented order of KSP and Unity: vessel modules, then part modules, then the controller component, then
`Physics.Simulate` with the vehicle update inside. The script order between the KSP part modules, the runtime-added `KSPWheelController`
and the `SuspensionLoadBalancer` VesselModule is project data, not C#, so it is assumed here. Each choice moves one input by one tick. The
call-trace instrumentation of the profiling branch can measure it (capture item C11c, section 5).

### 1.5 Quirks and surprises (a port must keep them)

| # | quirk | evidence | measured consequence |
|---|---|---|---|
| Q1 | `tireFriction.model = Lineal` never takes effect: `m_curve` stays the constructor's `SmoothFriction` | ModuleWheelBase.cs:361; TireFriction.cs:401-435 (`SetupFrictionCurves` only from the constructor and `OnAfterDeserialize`); no other caller in the assembly | Bug-E at Mun gravity: Smooth 4.54 m/s at 10 s vs Lineal 4.67 (3 %). No difference in the adherent or limit regime (`results/drive_mun_*`, `brake_flat_lineal`) |
| Q2 | VPP wheel inertia floor `I = max(0.01, m r^2 / 2)` | VehiclePhysics/Wheel.cs:76-84 (`Solver.minInertia`) | S2: I / r^2 = 0.41 t per wheel. The Bug-E accelerates as 4.53 t instead of 1.29 t. Without it the 0-10 m/s time would be 6.8 s instead of 21 s |
| Q3 | M1: collider radius x `rescaleFactor` (1.25 default, Part.cs:525), but the motor curve, `maxRpm` and slip displacement use the cfg radius | ModuleWheelBase.cs:333 vs :430, :453; ModuleWheelMotor.cs:142 | traction control reads a phantom 20 % slip: torque x (1 - 0.004 v) at 1x, x (1 - 0.016 v) at 4x; the M1 torque curve ends at 42.5 m/s true speed, not 34 |
| Q4 | the torque is rounded to 0.01 kN m | ModuleWheelMotor.cs:142-143 | the S2's top speed is 10.72 m/s (the curve's 12 m/s tail rounds to 0), in both the simulation and the reference |
| Q5 | auto-tuned spring: `k = springRatio x tweak x boost x (per-wheel load in g-tonnes)`, so the deflection `g0 / (springRatio x tweak x boost)` does not depend on mass or gravity; plus the PhysX rest offset `c0` | ModuleWheelSuspension.cs:388 + SuspensionLoadBalancer.cs:40-56 | LT-2 on Kerbin: k = 158 kN/m (tweak x boost about 1.7), 27.5 kN per leg -> 0.17 m + c0 0.12 m = 0.29 m measured (25 % of travel); the M1 sits at 49 % and the S2 at 53 % |
| Q6 | legs: `minimumDownforce = 10` kN and a 1000 kN m rest brake via `ModuleWheelLock` (the lock's own `maxTorque` is overridden once `brakeState` reaches 1) | ModuleWheelBase.cs:271-274; KSPWheelController.cs:256-263; ModuleWheelLock.cs:8-13 | a leg grips with at least 10 kN of load even when barely touching |
| Q7 | 100 % brakes switch to 1000 kN m: wheels lock, deceleration is set by the tyre curve (Kerbin auto-friction: peak mu 1.5 x 2.0 = 3.0) | KSPWheelController.cs:256-263; ModuleWheelBase.cs:382 | Prospector from 20 m/s: 53 deg nose-over (explicit), full flip (Jolt VC) |
| Q8 | `SuspensionLoadBalancer` rounds the per-wheel share to whole percent | SuspensionLoadBalancer.cs:55 | 8 wheels carry 8 x 12 % = 96 %, 6 wheels 6 x 17 % = 102 % |
| Q9 | traction control and tyre-force smoothing scale with `dt` | ModuleWheelMotor.cs:147, VehicleBase.cs:838-843 | behaviour changes with physics warp (section 4.5) |
| Q10 | steering is integrated at FRAME rate (`OnUpdate`, `Time.deltaTime`) | KSPWheelController.cs:230-238, VehicleBase.cs:1035-1073 | frame-rate dependence (PHYSICS-ENGINE-REVIEW 15). Not exercised by the spike |
| Q11 | `OnShouldIgnoreForces` (same-vessel contact) is assigned but never invoked; same-vessel filtering of the suspension ray relies on `CollisionManager.IgnoreCollidersOnVessel(vessel, wheelCollider)` instead | ModuleWheelBase.cs:324, :627; KSPWheelController.cs:64 | dead code; whether Unity's WheelCollider ray honours `Physics.IgnoreCollision` is native behaviour (the spike ignores the own vessel) |
| Q12 | `ModuleWheelSuspension.SuspensionSpringUpdate` uses the cfg (unscaled) `suspensionDistance` for `springTweakable` | ModuleWheelSuspension.cs:381 | rescaled parts tune with 1 / 1.25 of the travel |

### 1.6 Landing legs specifically

- **Deployment moves the collider.** `ModuleWheelDeployment` (ModuleWheels/ModuleWheelDeployment.cs:405-660) is a KerbalFSM over a Unity
  `Animation`. It moves the WheelCollider transform to the animated `deployTgt` transform (`SetWheelTransform`): while retracting every
  FixedUpdate (:440-473), while deploying every frame (`OnUpdate`, :526-543), another frame-rate dependence.
- **Partly deployed legs have no suspension.** Below `TsubSys` of the animation (LT-2: 0.83, LT-1: 1.0) the WheelCollider is destroyed
  and a `SphereCollider` stand-in of the wheel radius sits at `deployTgt` (:355-403).
- **Leg cfg values.** Leg suspensions are long and stiff-ish: LT-2 travel 0.95 x 1.25 = 1.19 m, `springRatio` 33, `damperRatio` 2.2.
  `useDistributedMass` is on.
- **The piston collider.** `piston_collider` (`suspensionColliderName`) is disabled at setup and re-enabled after 3 s.

## 2. Jolt implementation strategy

### 2.1 Where wheels live in the reduced model

Wheel and leg parts are welded into the vessel's cluster body with their mass. The welded wheel part carries no tyre collider: in KSP the
tyre is the WheelCollider ray, not a mesh. Each `ModuleWheelBase` part is a suspension element of that body:

- its WheelCollider transform is fixed in the body frame (for legs, animated by the deployment);
- per tick: a ray, a force at the contact point and a tyre force, all applied to the cluster body.

KSP applies them to the light wheel-part rigidbody, which a stiff joint and a ForceHeaviest autostrut tie to the vessel. The vessel motion
is the same; the wheel part's own jitter is gone (the "no wobble" decision of PHYSICS-SPIKE.md). WP-20c's structural graph must take the
suspension and tyre forces as external loads on the wheel part, so a wheel can still break off by joint overload.

### 2.2 Options compared

| | (A) custom ray + PhysX law, explicit (spike `explicit`) | (B) Jolt `VehicleConstraint`, suspension only (spike `jolt-vc`) | (C) Jolt `WheeledVehicleController` |
|---|---|---|---|
| suspension law | PhysX's exactly: `max(0, k (c - c0)) + d c'`; the damper can pull; explicit | Jolt soft constraint (implicit): spring `k (c - c0)` through a negative preload, push-only (spring and damper both clamped >= 0) | Jolt's |
| hard stop | PGS impulse rows | built in (`mSuspensionMaxUpPart`) | built in |
| tyre / drive | KSP's VPP (managed port) | KSP's VPP (null friction controller) | Jolt friction constraints + engine / transmission: **a different model** |
| touchdown 3 m/s (LT-2, Kerbin) | 50 % travel | 62 % travel (+23 %) | -- |
| rest at 1x | 0.3-0.5 mm braked drift | 0.9-2.2 mm | -- |
| warp 4x (dt 0.08) | unstable (lifts off / drifts) | stable suspension | -- |
| cost per wheel per tick | 0.32-0.44 us native | ~0.7 us inside the Jolt step | -- |
| faithful | yes (modulo the Unity glue, R1) | approximate | no |

(C) is rejected. KSP's tyre is VPP: slip-velocity curves with an adherent region, an explicit wheel-spin solver with 8 substeps and force
smoothing. Its drivetrain is one `DirectDrive` per wheel fed by a torque curve. Jolt's controller would replace both with different physics
and throw away a port that can be gated bit for bit.

**(A) is recommended**, for these reasons:

- It is the faithful law and the cheapest.
- Its one weakness, large-dt warp, is shared by the managed VPP code. The braked Bug-E creeps in (B) as well, from 2x up. So warp needs
  substeps whichever solver is used, and with substeps (A) is stable at every warp rate.
- (B) stays useful as the fallback if a later decision wants dt = 0.02 x rate for wheeled vessels. It would then need its own scenario
  bounds (+23 % touchdown compression).

### 2.3 Physics warp

KSP sets `Time.fixedDeltaTime = 0.02 x rate` (TimeWarp.cs:423), and the PhysX vehicle runs one explicit substep (VPWheelCollider.cs:197).

Measured:

- **Explicit suspension:** Bug-E and Prospector fail at 4x, the lander at 3x. For the Bug-E corner, `d dt / m = 5 x 0.08 / 0.16 = 2.5`,
  past the explicit-damper limit of 2. KSP's light wheel-part bodies are worse still, so KSP very likely bounces too. That is not
  measured: no capture.
- **VPP brake / tyre model:** a braked Bug-E creeps 1.0 / 3.0-3.4 / 7.8 m in 55 s at 2x / 3x / 4x in both variants. That is a limit
  cycle of the adherent tyre force with its `10 dt` smoothing.

**Recommendation for WP-10 / WP-20:** physics warp advances vessels with grounded wheels or legs in 0.02 s substeps, and the
simplest form is the whole bubble. PHYSICS-SPIKE.md 2.5 already proposed substepping for accuracy.

- Cost at 4x: 4 x 6-16 us per rover per frame tick.
- This is a deliberate change: KSP integrates at 0.08 s.

### 2.4 What WP-20b's `VesselPhysics` must give the wheel code

- **Per wheel part:** the cluster body, the part's pose in the cluster (the WheelCollider transform), the part's rb.mass (Unity's
  `sprungMass`, VPP's `referenceDownforce`), and the wheel part's `rb.velocity` / `GetPointVelocity` at the contact.
- **`RayCast`** with KSP's layer mask and the same-vessel filter: wheels ignore their own vessel's colliders and may hit other vessels.
  On a dynamic surface the reaction is `-F clamp01(m_other / m)` (VPWheelCollider.cs:371-379).
- **`AddForceAtPosition(part, F, p)` and an impulse / constraint hook** for the travel limit, inside a "vehicle update" slot of
  `PhysicsFixedUpdate` before the solver step.
- **A mutable compound or per-shape enable** for leg stand-in spheres and piston colliders. Wheel parts are welded without a tyre
  collider.
- **A vessel anchor (kinematic for 2 s)** for the brake "schloompa" freeze.
- **Geometry from the model reader** (the `.mu` files of `decompiled/KSP-dev`, CLAUDE.md "Original game assets"): per wheel part the
  `wheelCollider`, `wheelTransform` (WheelPivot), `suspensionTransform`, `deployTgt` transforms, the deploy animation clip, and the
  `piston_collider` / clip objects.

## 3. The prototype

Plain C++20 under `core/spikes/wheels/`. The target `wheel_spike` links kspcore and the vendored Jolt v5.6.0 (`DOUBLE_PRECISION`) and
reuses the Jolt spike's scene pieces: `ksp_context` (GameData mirror, Physics.cfg, Kerbol system, Kerbin PQS), `craft_model`,
`jolt_world` and `terrain_patch` (900 static quads from WP-11 heights). It is built only with `KSPCORE_BUILD_SPIKES=ON`. The default
build, the tests and CI are unchanged; the root CMakeLists gained one line inside the existing `if(KSPCORE_BUILD_SPIKES)` block.

| file | what it does |
|---|---|
| `src/wheel_model.*` | the managed port (section 1.3) in plain float32: TireFriction (Smooth / Lineal), Wheel + DirectDrive under the Euler solver, VehicleBase per-tick sequence, VPWheelCollider managed parts, KSPWheelController, ModuleWheelBase / Suspension / Motor / Brakes / Lock / Damage (diagnostic), SuspensionLoadBalancer. Module fields come from the live kspcore Part (cfg defaults + craft persistent values) |
| `src/rover.*` | the reduced vessel: one Jolt body (KSP rb masses, inertia with the per-part `max(1, m)` floor, drag-cube box compound without wheel parts), gravity + Coriolis + centrifugal in Kerbin's double body-fixed frame. The two native variants (`explicit` / `jolt-vc`, the latter with a `NullController`), and the tick in KSP's order (section 1.4) |
| `src/scenarios.*` | rest, drive, brake, touchdown and cost benches, plus the analytic references: the C# motor model without slip on the vessel mass + wheel spin inertia; the C# brake torque capped by the tyre curve |
| `tools/run_benches.sh` | every set -> `results/` (20 s wall) |

Craft are stock files from `harness/inputs/Ships`: Bug-E Buggy (SPH, 26 parts, 1.287 t, 8 x roverWheel2, 6 motors enabled, brakes 100 %),
Prospector Rover (SPH, 19 parts, 1.249 t, 4 x roverWheel1, 2 motors enabled, brakes 100 %), Two-Stage Lander (VAB, 45 parts, 11.22 t,
4 x landingLeg1-2), and the Crater Crawler (115 parts, 10 x wheelMed) for cost. Each is loaded through `KSPUpgradePipeline.Process` +
`ShipConstruct.LoadShip` + `AssembleVesselFromCraft`, as the WP-18 gate does.

**Spike simplifications (WP-29 must not keep them):**

- **Geometry proxies** (no `.mu` in the repository): the tyre bottom at full extension is the bottom face of the part's drag cube.
  For deployable parts that is the "Deployed" cube; `--leg-offset` uses KSP's `groundHeightOffset` instead. The suspension axis is the
  craft up axis, and the rolling direction is the root part's up projected on the ground plane.
- **Wheels and inputs held fixed:** no steering, motor orientation sign +1, electric charge always available, legs fixed at Deployed.
- **No aero**, and no Mono float rule.
- **Frame handling:** SPH craft come out of `AssembleVesselFromCraft` in the root part's frame, so the craft up axis is taken as
  `inverse(root file rotation) * Y` (the root probe of an SPH craft is rotated 90 deg about X).

## 4. Measurements

All runs are single-threaded and deterministic: two separate runs of the 1x rest set gave identical values in all 12 rows, every column but
timing. The cost runs are pinned to logical processor 2 (`--affinity 4`), best of 3.

### 4.1 Rest on the pad, 60 s (`results/rest.md`)

Drift is the CoM displacement from t = 5 s to 60 s.

| craft | native | brakes | 1x horiz / max speed | 2x | 3x | 4x |
|---|---|---|---|---|---|---|
| Bug-E | explicit | on | 0.3 mm / 0.005 m/s | 1.57 m | 3.00 m | lifts off (0/8 grounded), 10 m |
| Bug-E | jolt-vc | on | 0.9 mm / 0.008 m/s | 0.98 m | 3.37 m | 7.8 m |
| Prospector | explicit | on / off | 0.5 / 7.2 mm | 0.5 / 8.5 mm | 0.6 / 9.0 mm | 5.9 / 100.7 m |
| Prospector | jolt-vc | on / off | 1.1 / 7.3 mm | 0.5 / 8.6 mm | 0.6 / 9.8 mm | 0.8 / 10.5 mm |
| Two-Stage Lander | explicit | (locked) | 0.5 mm | 3.8 mm | 18.7 m | 0.85 m, 1/4 legs |
| Two-Stage Lander | jolt-vc | (locked) | 2.2 mm | 1.3 mm | 0.9 mm | 5.5 mm |

Settled compression at 1x, as a fraction of travel: Bug-E 0.53 (S2), Prospector 0.49 (M1), lander 0.25 (LT-2). Unbraked rovers creep
5.8-7.3 mm in 55 s, the centrifugal component described in the Summary.

### 4.2 Drive under motor torque (`results/drive_flat.md`, `drive_pqs.md`)

Full throttle after a 2 s settle on a flat plane. The reference is the C# motor model on the same tick (drive ramp, rounded torque curve,
traction control, the craft's enabled motors) without tyre slip, accelerating the vessel mass plus the wheels' spin inertia.

| craft | native | warp | v at 10 / 20 / 30 / 40 s (m/s) | reference | max forward slip |
|---|---|---|---|---|---|
| Prospector (m 1.249, m_eff 1.428 t) | explicit | 1x | 19.41 / 28.36 / 33.36 / 37.01 | 19.41 / 28.35 / 33.40 / 37.15 | 0.00 |
| | jolt-vc | 1x | 19.42 / 28.36 / 33.36 / 37.00 | same | 0.00 |
| | explicit | 4x | 15.01 / 23.90 / 27.09 / 29.73 | 17.77 / 25.40 / 29.64 / 32.49 | 30.8 (bouncing) |
| | jolt-vc | 4x | 17.84 / 25.42 / 29.59 / 32.36 | same | 0.00 |
| Bug-E (m 1.287, m_eff 4.533 t) | explicit | 1x | 7.95 / 9.89 / 10.71 / 10.71 | 7.36 / 9.70 / 10.55 / 10.71 | 0.05 |
| | jolt-vc | 1x | 8.03 / 9.87 / 10.71 / 10.71 | same | 0.01 |
| | jolt-vc | 4x | 8.77 / 10.34 / 11.04 / 10.74 | 7.39 / 9.71 / 10.56 / 10.72 | 0.25 |

- **The PQS patch** (start 700 m west of the pad, heading east, 30 s) gives the flat-plane numbers within 0.1 m/s.
- **Driving off the 30 s window:** past ~730 m the terrain drops ~2 m (plus 0.68 m of curvature over 900 m), and a Prospector at
  33 m/s jumps and rolls.
- **Mun gravity** (`drive_mun_*`): the Prospector is traction-limited (17.8 m/s at 40 s vs 30.4 slip-free, 12 m/s wheel spin).
- **Bug-E at 4x with jolt-vc** accelerates 19 % faster than its 4x reference to 10 s. VPP's coupling between wheel spin and the
  vehicle (the `num3 = (V / r - w) I / dt` torque limits) is dt-sensitive: one more reason for substeps.

### 4.3 Brakes (`results/brake_flat*.md`)

The brake action group is applied with the craft's `brakeTweakable` (100 % in both rovers). That locks the wheels after 1 s
(`brakeResponse` 1) through the 1000 kN m rest torque.

| craft | v0 | explicit: distance / time / max tilt | jolt-vc | reference (torque, then tyre curve) | locked from t = 0 |
|---|---|---|---|---|---|
| Bug-E | 8 m/s | 8.30 m / 1.30 s / 12 deg | 8.20 m / 1.32 s / 16 deg | 8.04 m / 1.38 s | 1.23 m |
| Prospector | 8 m/s | 7.34 m / 1.22 s / 11 deg | 7.39 m / 1.28 s / 17 deg | 6.05 m / 1.22 s | 1.23 m |
| Prospector | 20 m/s | 23.6 m / 2.1 s / 53 deg | 31.4 m / 6.6 s / flipped (179 deg) | 21.7 m / 1.68 s | 7.7 m |

The remaining gap to the reference is load transfer and pitch. The reference has none, and the locked tyres reach mu 2.4-2.9 on the
loaded front wheel. Lineal instead of Smooth changes nothing here.

### 4.4 Landing-leg touchdown (`results/touchdown*.md`)

Two-Stage Lander, legs deployed, engines off, starting 2 cm above the ground at -3 m/s, 20 s.

| leg proxy | native | g | max compression (travel / m) | max rebound speed | legs off ground | bounce-launch | settle | body strike |
|---|---|---|---|---|---|---|---|---|
| drag cube (default) | explicit | Kerbin | 0.50 / 0.596 | 1.87 m/s | never | no | 1.42 s | no |
| | explicit | Mun 0.166 | 0.61 / 0.725 | 0.53 m/s | never | no | 2.56 s | yes (ladder, 0.72 m margin) |
| | jolt-vc | Kerbin | 0.62 / 0.733 | 1.71 m/s | never | no | 1.52 s | yes |
| | jolt-vc | Mun | 0.65 / 0.776 | 0.41 m/s | never | no | 2.68 s | yes |
| `groundHeightOffset` | explicit / jolt-vc | both | 0.22-0.40 | <= 0.35 m/s | never | no | 0.8-1.8 s | yes, at first contact (engines and ladder carry the vessel) |

- PQS and flat ground agree to 1 cm.
- ModuleWheelDamage impact stress is at most 0.09 of tolerance, so no leg would break.
- The legs bounce back hard: about 62 % of the impact speed, damping ratio about 0.45 from KSP's `sqrt(k ratio)` damper.
- The vessel never leaves the ground, and it never rises above its first-contact height.

### 4.5 Physics warp

Covered in section 2.3 and the tables above: the explicit suspension fails at 3-4x, the VPP brake / tyre model creeps from 2x, and Jolt's
implicit suspension holds to 4x.

### 4.6 Cost (`results/cost_pinned.md`)

Rovers at full throttle, the lander at rest. Times are ms per tick (20 s of game time, best of 3); `us/wheel` is managed + native per wheel.

| craft | wheels | native | managed (8 substeps) | native | Jolt step | tick | us/wheel |
|---|---|---|---|---|---|---|---|
| Bug-E | 8 | explicit | 0.0035 | 0.0032 | 0.0064 | 0.0131 | 0.83 |
| Bug-E | 8 | jolt-vc | 0.0037 | 0.0001 | 0.0118 | 0.0157 | 0.48 + ~0.7 in step |
| Prospector | 4 | explicit | 0.0017 | 0.0017 | 0.0054 | 0.0090 | 0.87 |
| Crater Crawler | 10 | explicit | 0.0028 | 0.0031 | 0.0043 | 0.0104 | 0.60 |
| Two-Stage Lander | 4 | explicit | 0.0017 | 0.0017 | 0.0026 | 0.0061 | 0.85 |

With 4 VPP substeps (KSP's inactive vessels) the managed part drops to 0.17-0.29 us per wheel. A wheel costs about one part of the
reduced model's per-part pipeline (0.4-0.9 us, PHYSICS-SPIKE.md 2.7). Even a 10-wheel rover adds less than 0.01 ms per tick. No KSP
number exists to compare: no wheeled craft was profiled (PHYSICS-ENGINE-REVIEW.md 12).

## 5. Comparison with KSP

**No in-game capture covers wheel dynamics.** `harness/captures/out` and `out2` contain no wheel tick. PHYSICS-ENGINE-REVIEW.md 12:
"Wheels and legs at runtime: no wheeled craft was flown."

What is covered, and therefore already bit-exact for the port's inputs:

- the wheel and leg parts' module fields: `out2/part_database.json`, 26 ModuleWheelBase / ModuleWheelSuspension / ModuleWheelDamage
  rows and the rest;
- the wheels' torque and steering FloatCurves evaluated natively: `out/curves_eval_native.json`. kspcore's FloatCurve is 0 ULP against it.

Every dynamic number above therefore compares against the C# reading (the analytic references) or between the two native variants, not
against the game.

To close the gap, capture **C11 "wheels"**. It needs the profiling branch (`decompiled/_archive`, not on this PC) and is local only:

- **(a)** Per FixedUpdate, for every wheel of the Prospector Rover, Bug-E Buggy and Two-Stage Lander, in scripted scenes (pad settle 10 s,
  full throttle 20 s, Brakes, a 3 m/s drop of the lander via the cheat SetPosition), record:
  - `GetGroundHit` (point, normal, force, forwardSlip, sidewaysSlip);
  - `WheelCollider` (spring, damper, targetPosition, suspensionDistance, center, radius, sprungMass, forceAppPointDistance);
  - VPP `WheelState` (contactDepth, downforce, localWheelVelocity, angularVelocity, tireForce, reactionTorque, drive / brake torque);
  - module state (spring, damper, boost, autoBoost, sprungMassGravity, driveInput);
  - the wheel part's and the vessel's rigidbody velocity, position and rotation;
  - `Physics.gravity` and `Time.fixedDeltaTime`.
- **(b)** Reflection on `tireFriction.m_curve.GetType()` (Q1).
- **(c)** The call trace of ModuleWheel* / KSPWheelController / SuspensionLoadBalancer / VPWheelCollider relative to the physics step.
- **(d)** (a) at 2x and 4x physics warp.

(a) turns the native law, the target mapping and the WheelHit timing into a capture gate. Over a tick the velocity change of the wheel part
gives the applied suspension impulse, including the limit constraint. (a) also makes WP-29a/b's reference a capture.

## 6. Risks

| # | risk | evidence | mitigation | owner |
|---|---|---|---|---|
| R1 | **Unity glue is unverified**: targetPosition mapping, sprungMass, forceAppPoint, `WheelHit.force == suspSpringForce`, the plane-intersection jounce on slopes (PhysX intersects the hit plane with the wheel, not the bare ray), the limit constraint's stiffness, the one-tick-stale hit | PhysX 4.1 source pins the law; Unity docs pin the target direction; VPP's own formulas are consistent with both | capture C11a; until then gate by scenario; the native model is ~200 lines, cheap to correct | WP-29c / WP-29e |
| R2 | **Tyre curve class** (Smooth by the C# reading, Lineal intended) | Q1; 3 % acceleration effect in the 0.5-3 m/s slip band | port both curves (the spike has them), default Smooth, switch on capture C11b | WP-29a |
| R3 | **Physics warp**: explicit suspension unstable from 3-4x, VPP brake / tyre limit cycle from 2x | section 2.3 | 0.02 s substeps for vessels with grounded wheels (decision for WP-10 / WP-20); Jolt VehicleConstraint as fallback | WP-10 / WP-20 / WP-29c |
| R4 | **Geometry from `.mu` files**: wheelCollider / deployTgt transforms, the deploy animation, piston and stand-in colliders | LT-2: drag-cube vs `groundHeightOffset` proxies differ by 0.85 m and flip the touchdown between a clean landing and an engine strike | the model reader extracts the transforms and clips (the WP-17b `mu_transforms.py` route); WP-29c's scenario gates run on real geometry only | UE import / WP-29c |
| R5 | **Script order** (modules vs controller vs load balancer vs the PhysX vehicle step) unknown | section 1.4 | capture C11c; the spike's order is the documented Unity order | WP-29b |
| R6 | **No wheel flex**, and the wheel attach joint load must still be checked | reduced model; KSP breaks wheels off by joint overload | WP-20c graph: suspension + tyre forces as external loads on the wheel part; ModuleWheelDamage stays as is | WP-20c / WP-29c |
| R7 | **Chaotic manoeuvres diverge** between equally valid solvers | Prospector 100 % brakes at 20 m/s: 53 deg vs a full flip | gate violent scenarios by envelope (stops, does not explode, tilt bound), never by trajectory | WP-29c |
| R8 | **Faithful quirks look like bugs** (wheel inertia floor, phantom slip of rescaled wheels, torque rounding, brake rest torque, 96 % load share) and invite "fixes" | Q2-Q8 | the bit-exact gates of WP-29a/b pin them; STATUS lists them as quirks | WP-29a/b |
| R9 | **Frame-rate steering** | Q10 | WP-00 PlayerLoop Update phase; deterministic tests at a fixed frame rate | WP-29b |
| R10 | **Not prototyped**: steering, aircraft landing gear on the runway (needs aero, engines, runway colliders), wheels on another vessel (reaction force), docking / undocking resets, EVA construction, retract / extend transitions, repair | -- | WP-29d scenarios; the code paths are managed and ported in 29a/b | WP-29b / 29d |

## 7. WP-29 package split and gates

WP-29's file list is 163 files / 29,217 lines; 34 files / 8,867 lines are reachable from KSP's wheel runtime.

- **Drop:** the other ~20.4k lines (Edy's VPVehicleController, Engine, Gearbox, Driveline, Differential*, Clutch, Retarder, VPAudio,
  VPDamage, VPTelemetry, DataLogger, DataRecorder, charts, cameras, cargo, VPVehicleJoint, EdyCommonTools TextureCanvas / GColor /
  MiniJSON / UDP / CsvFile / Spline ...). Nothing in Assembly-CSharp outside `VehiclePhysics/` and `EdyCommonTools/` references them;
  the one exception is `MathUtility.ClampAngle`, used by a debug cheat screen. Record them as "not ported: unreachable".
- **Also drop:** the root `Wheel.cs` (only `TestVehicleController`) and `ModuleWheels/WheelFrictionNode.cs` (no user).

"Bit-exact" means 0 ULP against an independent Python reference written from the C# (the WP-21 ... WP-55 method), with the Mono float rule.
"Scenario" means a headless run with stated bounds.

| package | content (C# files) | depends on | gate: bit-exact | gate: scenario |
|---|---|---|---|---|
| **WP-29a VPP wheel core** (~4.2k C# lines) | VehiclePhysics/VehicleBase, VehicleBehaviour, VPWheelCollider (managed parts), Wheel, DirectDrive, Block, Solver (Euler + RK4), TireFriction (all five models), DataBus, GroundMaterial*; KSPWheelController, KSPWheelGravity; MathUtility / InterpolatedFloat / ColliderUtility subsets. Pure functions over (WheelHit, rigid-body state, inputs) -> (tyre force, spin, native spring writes) | WP-01 (FloatCurve, Mono rule) only: **can start now** | 10,000+ ticks per stock wheel type, synthetic native inputs (grounded / airborne transitions, slopes, moving surfaces, drive / brake / lock / reverse, 8 and 4 substeps), every output of `VehicleBase.FixedUpdate` and `UpdateWheelCollider` 0 ULP vs the Python reference; Smooth and Lineal | the spike's drive reference: slip-free motor + wheel-inertia model vs the 29a block on a 1-D vehicle, within 1 % |
| **WP-29b wheel part modules** (~4.6k) | ModuleWheelBase (gee bias, friction, landing detection -> GroundContacts / landedAt, slip displacement, schloompa event, drift fix, OnPutToGround, events / actions), ModuleWheels/* (Suspension + boost, Motor + EC via WP-15, MotorSteering, Steering / Ackermann, Brakes + WP-55 adjusters, Lock, Deployment FSM on WP-27's animation, Damage + repair, Bogey, Subsystem(s), WheelUtil), SuspensionLoadBalancer, WheelType; Vessel.cs wheel bits (anchor exceptions :1565, ground-height wheel collider :2615-2640) | WP-06, WP-13/14, WP-15, WP-27, WP-29a | module formulas and state machines vs the Python reference on recorded scenes, 0 ULP; `capture.part_database` unchanged; persistent fields round-trip on the craft / save corpus | -- |
| **WP-29c native WheelCollider + vessel integration** (~1.5-2.5k new C++) | the ray query (layer mask, same-vessel rule), the PhysX 4.1 law with Unity's mapping, jounce memory, the travel-limit constraint, WheelHit assembly (+ v dt, PhysX lateral slip), reaction on dynamic surfaces, wheel parts welded into the cluster, leg stand-in spheres / piston colliders, the schloompa anchor, physics-warp substepping; the spike's `rover.cpp` explicit path is the starting point | WP-20a (done), **WP-20b**, WP-29a/b; the model reader for geometry | the suspension law and limit vs the formula on synthetic compressions (exact); WheelHit assembly exact; determinism (bit-identical twice) | on real geometry: `wheel_rest` 60 s braked drift < 5 mm at 1x and identical at 4x (substeps); `wheel_drive` vs the 29a reference within 2 % (Prospector-class) / 10 % (Bug-E-class) at 10-40 s, top speed within 1 %; `wheel_brake` 8 m/s stop within 25 % of the reference, never explodes, tilt < 30 deg; `leg_touchdown` 3 m/s: no bounce-launch, compression 0.35-0.75 of travel, settles < 3 s; then capture gates from C11 |
| **WP-29d gear, steering, aircraft** | stock landing gear (GearSmall / Medium / Large / ExtraLarge / Free / Fixed, SmallGearBay), runway take-off / landing, steering circles, retract / extend in motion | WP-29c, WP-20d (aero), WP-23, WP-12b / WP-42 (runway colliders) | Ackermann turn-centre geometry vs the C# (part of 29b) | `runway_roll`: Aeris 3A take-off speed and distance within 10 % of a C# + aero reference; `steer_circle` radius vs geometry within 5 %; gear retract in flight does not touch the ground |
| **WP-29e capture C11 (local)** | profiling-branch capture of section 5 | the `decompiled/_archive` profiling branch on this PC | -- | turns 29c's scenario gates into capture gates where KSP is deterministic (pad rest, straight drive, lander drop) |

**Order:** 29a now. 29b after 29a. 29c after WP-20b, with the model reader in parallel. 29d after 20d / 23 / 12b. 29e whenever the
profiling branch is restored on this machine. **Implementer fit:** 29a and 29b are Sonnet `implementer` packages (managed port with a
Python reference); 29c is `optimizer` or an implementer starting from the spike code.

## 8. Reproducing

```powershell
# from core/ (worktree or main checkout); CMAKE_BUILD_PARALLEL_LEVEL=6
powershell -NoProfile -ExecutionPolicy Bypass -File spikes\wheels\build_spike.ps1      # build/spike-jolt, target wheel_spike
build\spike-jolt\spikes\wheels\wheel_spike.exe info bug-e prospector twostage            # wheel parameters, proxies, masses
build\spike-jolt\spikes\wheels\wheel_spike.exe trace twostage --mode drop --v0 3 --every 0.1 --all-wheels
build\spike-jolt\spikes\wheels\wheel_spike.exe trace bug-e --mode drive --ground flat --wheel 0 --from 2 --to 3   # one wheel's internals per tick
bash spikes/wheels/tools/run_benches.sh build/spike-jolt                                  # every set -> spikes/wheels/results (20 s)
```

Options: `--native explicit|jolt-vc|both`, `--ground pqs|flat`, `--friction smooth|lineal`, `--warp <rate>` (trace) / `--warps 1,2,4`
(rest), `--warp4` (drive), `--g <scale>`, `--heading <deg>`, `--east / --north <m>`, `--leg-offset`, `--no-inertia-floor`,
`--crafts a,b`, `--v0`, `--seconds`, `--affinity <hex>`, `--repeat <n>`. Craft aliases: `bug-e`, `prospector`, `crawler`,
`twostage`, `munsplorer`, `skycrane` (`harness/inputs/Ships`), or a path. Nothing reads `decompiled/KSP-dev`. Game data comes from
`core/testdata/GameData-mirror`, terrain from `core/data/terrain`, and the upgraded craft are written to the build tree.
