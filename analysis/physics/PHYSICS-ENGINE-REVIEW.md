# KSP 1.12.5 physics and flight simulation: engine review

Status: 2026-10-08. Companion measurement: [PROFILE.md](PROFILE.md).

**Scope.** This document describes how KSP 1.12.5 simulates a vessel in flight: the per-tick pipeline, rigid-body structure, forces, thermal, precision frames, time warp, wheels and terrain. It also maps what is tied to Unity and ranks the performance changes worth making in a reimplementation.

**Sources and citation format.**
- Every claim is cited as `Assembly-CSharp/<file>.cs:<line>`, relative to `C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\`. Line numbers are against branch `main` (commit `4e54f682`), the clean decompilation.
- The `profiling` branch adds instrumentation and shifts line numbers, so do not cite against it.
- Config values are cited as `KSP-dev/<file>:<line>`, meaning `C:\Projects\KSP-UE5\decompiled\KSP-dev\`.
- Values marked **[runtime]** were read from the live game by the profiling driver (`analysis/physics/raw/run1/*_summary.csv`, keys `rt_*`). This matters for fields that Unity serializes into scenes, where the C# initializer is not the value the game actually uses.

**Contents**
1. [The per-tick pipeline](#1-the-per-tick-pipeline)
2. [Rigid-body structure](#2-rigid-body-structure)
3. [Forces](#3-forces)
4. [Thermal](#4-thermal)
5. [Precision, frames, rails and the physics bubble](#5-precision-frames-rails-and-the-physics-bubble)
6. [Time warp](#6-time-warp)
7. [Wheels, legs and terrain collision](#7-wheels-legs-and-terrain-collision)
8. [Unity-coupling map](#8-unity-coupling-map)
9. [Quirks found in the code](#9-quirks-found-in-the-code-decide-per-item-whether-to-reproduce)
10. [What "faithful" means](#10-what-faithful-means)
11. [Performance improvement opportunities, ranked](#11-performance-improvement-opportunities-ranked)
12. [Unverified items](#12-unverified-items)

---

## 1. The per-tick pipeline

### 1.1 Execution model

KSP has no simulation loop of its own. Each class runs from its own Unity callback (`FixedUpdate`, `Update`, `LateUpdate`, coroutines), so the order of work inside a tick is Unity's **script execution order**. That order is a project setting serialized in the game data, not code: the decompiled assembly contains no `[DefaultExecutionOrder]`.

The `TimingManager` stages (`ObscenelyEarly, Early, Precalc, Earlyish, Normal, FashionablyLate, FlightIntegrator, Late, BetterLateThanNever`, `Assembly-CSharp/TimingManager.cs:6-17`) are only empty hook components (`Timing0..5`, `TimingPre`, `TimingFI`), placed in the order by that setting. Stock code registers only two LateUpdate hooks with them (`Assembly-CSharp/CommNet/CommNetUI.cs:399`, `Assembly-CSharp/MapView.cs:225`).

**The order below was measured, not inferred.** The profiling build records, in execution order, every instrumented callback of one frame during powered ascent (`analysis/physics/raw/run1/run1-M_calltrace.csv`, 150-part craft, Krakensbane engaged). One measured surprise: the stage hooks do **not** run in enum order. `TimingPre` (Precalc) runs before `Timing1` (Early).

**Tick rate.**
- `Time.fixedDeltaTime = 0.02·timeScale` (`Assembly-CSharp/TimeWarp.cs:423`).
- `Time.maximumDeltaTime = PHYSICS_FRAME_DT_LIMIT = 0.04` [runtime] (`Assembly-CSharp/GameSettings.cs:2016`).
- So a rendered frame runs **0, 1 or 2** physics ticks. Above 50 FPS most frames run none; once frames take more than 40 ms, game time slows down.
- The whole FixedUpdate group below repeats for each tick.

### 1.2 Call order of one physics tick: off-rails vessel in atmosphere (measured)

Times are inclusive µs, from the traced frame of the 150-part craft (`run1-M`, 15 s into the ascent window, subsonic, below 5 km). Sub-phase values marked ~ are per-tick averages over the whole ascent window (PROFILE.md table C), not the single traced tick.

```
FixedUpdate group (once per 0.02 s of game time; 0-2 times per rendered frame)
│
├─ ScriptRunBehaviourFixedUpdate  — Unity script execution order, measured
│   1  Timing0                                ObscenelyEarly hook (no stock users)
│   2  Planetarium.FixedUpdate           83   UT += dt; CBUpdate on every body (rotation, body OrbitDriver.UpdateOrbit)
│   3  VesselPrecalculate.FixedUpdate    21   per vessel (2): QueueOnce; UPDATE-mode orbit or SetLandedPosRot;
│                                             ApplyVelocityCorrection; CalculateGravity -> precalc.integrationAccel
│                                             (full CalculatePhysicsStats only if no Update ran since last tick); CheckKill
│   4  TimingPre                              Precalc hook
│   5  OrbitDriver.FixedUpdate x18        1   no-op for vessels (QueueOnce) and bodies (QueuedUpdate)
│   6  Timing1                                Early hook
│   7  CollisionEnhancer.FixedUpdate x150  194   per part: Physics.Linecast vs layer 15 if moved > 0.1 m
│   8  Timing2, TimingManager                 Earlyish, Normal hooks
│   9  FlightGlobals.FixedUpdate          7   active-vessel bookkeeping
│  10  PQS.FixedUpdate x18               13   sphere transform caches
│  11  FlightInputHandler.FixedUpdate   180   Vessel.FeedInputFeed: OnPreAutopilotUpdate -> SAS PIDs ->
│                                             OnAutopilotUpdate -> OnFlyByWire -> ctrlState
│  12  TimeWarp.FixedUpdate                   rate lerp and warp limits
│  13  default-order block, component registration order, interleaved per part:
│        Part.FixedUpdate x150         337    ValidateInertiaTensor; mass (UpdateMass, or ModulesOnFixedUpdate
│                                             if the part is ACTIVE); crew; heat gauge; temperature,
│                                             pressure and g limits
│        ModuleEngines.FixedUpdate x17 416    throttle -> RequestPropellant -> thrust ->
│                                             part.AddForceAtPosition (buffered); exhaust Physics.Raycast
│        ModuleGimbal, FXModuleAnimateThrottle, ModuleAlternator, ModuleReactionWheel
│          (part.AddTorque, buffered), ModuleSAS, ModuleCommand ...                      ~90
│        PartBuoyancy.FixedUpdate x150  235   per part: water test
│        DestructibleBuilding x39, KbApp_*, SuspensionLoadBalancer, SunCoronas            ~10
│  14  Timing3                                FashionablyLate hook
│  15  FlightIntegrator.FixedUpdate    1017   per loaded vessel:
│        UpdateMassStats (resource mass, thermal mass, rb.mass, rb.centerOfMass)          ~220
│        SmoothVelocity -> CalculatePressure -> CalculateConstantsAtmosphere (q, Mach, rho, h_conv)
│        DragCubeSetupAndPartAeroStats                                                     ~40
│        CheckThermalGraph -> [UpdateOcclusion(all) if flagged]
│        UpdateThermodynamics: conduction + per-part convection/radiation                  ~240
│        [packed -> clear force buffers, return]
│        IntegratePhysicalObjects (debris, active vessel only); OffsetVelocity (drift compensation)
│        Integrate(root), recursive:                                                       ~520
│          rb.AddForce(gravity, Acceleration); rb.AddForce/AddTorque/AddForceAtPosition(buffers)
│          UpdateAerodynamics(part): drag-cube drag + body lift -> rb.AddForceAtPosition
│  16  TimingFI                               FlightIntegrator hook
│  17  OrbitPhysicsManager.FixedUpdate    1   checkReferenceFrame (rotating <-> inertial switch)
│  18  Vessel.FixedUpdate x2              1   UpdateCaches, frame counters
│  19  VesselDeltaV, Timing4                  Late hook
│  20  Krakensbane.FixedUpdate           29   if |v| > 5 m/s and safe: AddExcess -> every part rb.velocity -= v;
│                                             FrameVel += v; FloatingOrigin.SetOutOfFrameOffset(FrameVel*dt)
│  21  FloatingOrigin.FixedUpdate      1152   origin shift every tick while FrameVel != 0: moves all bodies,
│                                             all vessels (part transforms), every live particle,
│                                             physical objects, terrain shader offset
│  22  Timing5, CollisionManager.FixedUpdate   BetterLateThanNever hook; rebuild IgnoreCollision pairs if flagged
│
├─ PhysicsFixedUpdate                 ~2100   PhysX simulate(0.02): 50 position / 1 velocity solver iterations
│                                             [runtime]; then OnCollisionEnter/Stay/Exit and OnJointBreak dispatch
└─ ScriptRunDelayedFixedFrameRate             WaitForFixedUpdate coroutines (e.g. SecureAutoStruts)

Update group (once per rendered frame, after all ticks of the frame)
├─ ScriptRunBehaviourUpdate: Planetarium.Update -> VesselPrecalculate.Update (205 µs: CalculatePhysicsStats
│     = CoM/velocity/MOI from rigidbodies; OrbitDriver.UpdateOrbit -> TrackRigidbody = orbit rebuilt from
│     rb state; UpdatePosVel; UpdateAcceleration) -> ... -> Part.Update xparts (ModulesOnUpdate) ->
│     FlightIntegrator.Update (sun Physics.Raycast; occlusion round-robin) -> Vessel.Update (load/unload ranges) -> UI
└─ ScriptRunDelayedDynamicFrameRate: coroutines, including PQS.UpdateSphere (terrain quads + MeshColliders)
PreLateUpdate: LateUpdate scripts (OrbitPhysicsManager pack/unpack, Vessel situation/landed anchor, FlightCamera, UI)
PostLateUpdate: canvases, renderers, cameras, present
```

### 1.3 What the measured order implies for data flow

- **Gravity** is computed at the start-of-tick CoM (step 3). It is applied in step 15, before any position change, so it is a pure explicit evaluation.
- **Buffered module forces apply in the same tick.** Thrust, wing lift and drag, RCS, reaction-wheel torque and buoyancy are buffered in step 13 and applied by `Integrate` in step 15. There is no one-tick lag, which the C# alone could not rule out (§3.1).
- **Wings read last tick's dynamic pressure.** `ModuleLiftingSurface` (step 13) reads `part.dynamicPressurekPa`, which `FlightIntegrator` only recomputes in step 15. Engine Isp and flow likewise use last tick's `part.staticPressureAtm`.
- **Thermal uses last tick's exposed areas.** Within step 15, `UpdateThermodynamics` runs before `Integrate -> UpdateAerodynamics` refreshes `exposedArea` (§4.1).
- **Krakensbane and FloatingOrigin run after all forces are queued** (steps 20-21), immediately before PhysX. The velocity frame and the origin therefore change between force accumulation and integration. Queued forces are frame-invariant, so this is harmless.
- **PhysX is the only integrator for off-rails vessels.** It does symplectic Euler over all part bodies and joints. KSP code never integrates positions itself.
- **The double-precision orbit is rebuilt from the PhysX result** in the next frame's `VesselPrecalculate.Update -> TrackRigidbody` (§5.3).
- **Transform writes are synced before every physics query.** `Physics.autoSyncTransforms = True` [runtime]: every Transform write in steps 1-22 (Krakensbane velocity writes, FloatingOrigin moves, wheel transforms) is synced before the next physics query. That raises the cost of the first raycast after a shift.

---

## 2. Rigid-body structure

### 2.1 One PhysX Rigidbody per physically significant part

**Created in the `Part.Start` coroutine.** There is no separate setup method.
- After the first `yield return null`, every part with `physicalSignificance == FULL` gets (or reuses) a `Rigidbody` (`Assembly-CSharp/Part.cs:1868-1875`).
- Rigidbody settings:
  - `useGravity = false` and `drag = 0` (`Assembly-CSharp/Part.cs:1876-1877`).
  - The auto-computed centre of mass is frozen with `rb.centerOfMass = rb.centerOfMass` (`Assembly-CSharp/Part.cs:1878-1881`).
  - `isKinematic = packed` (`Assembly-CSharp/Part.cs:1882`).
  - `maxAngularVelocity = PhysicsGlobals.MaxAngularVelocity` (`Assembly-CSharp/Part.cs:1883`). That is 50 rad/s in `KSP-dev/Physics.cfg:91`; the code default is 31.416 (`Assembly-CSharp/PhysicsGlobals.cs:383`).
- Two more MonoBehaviours are added to the same GameObject: `CollisionEnhancer` and `PartBuoyancy` (`Assembly-CSharp/Part.cs:1884-1886`). Each has its own Unity `FixedUpdate`, so every physical part costs at least three Unity FixedUpdate dispatches per tick: Part, CollisionEnhancer and PartBuoyancy.
- After a second yield, the normalised inertia tensor is cached (`rb.inertiaTensor / max(1, rb.mass)`) and the attach joint is created (`Assembly-CSharp/Part.cs:1889-1895`).
- **Physical parts are unparented** in flight (`transform.parent = null`), so the flight Transform hierarchy is flat (`Assembly-CSharp/Part.cs:1835-1838`).

**Settings KSP never makes.**
- KSP never sets `interpolation`, `collisionDetectionMode` or `solverIterations` on part bodies; the only hits are in the bundled VehiclePhysics.
- PhysX defaults therefore apply. **[runtime]** values on a part body:
  - `interpolation = None`
  - `collisionDetectionMode = Discrete`
  - `solverIterations = 50` and `solverVelocityIterations = 1`. KSP's project-wide `Physics.defaultSolverIterations` is 50, against Unity's stock default of 6.
  - `maxDepenetrationVelocity = 1e32`, i.e. unlimited, which is why interpenetration produces violent ejections.
  - `sleepThreshold = 0.005`
- Other project-wide values **[runtime]**: `Physics.gravity = (0,0,0)`, `defaultContactOffset = 0.01`, `bounceThreshold = 0.2`, `autoSyncTransforms = True`, `reuseCollisionCallbacks = False`.
- Tunnelling through terrain is handled by KSP's own `CollisionEnhancer` (§7.4).

**Per-tick mass and inertia upkeep (O(parts) every tick).**
- `Part.FixedUpdate` → `ValidateInertiaTensor()` (`Assembly-CSharp/Part.cs:4159`, `:4263-4273`). It rewrites `rb.inertiaTensor = cachedTensor * max(1, rb.mass)` whenever that value changed.
  - A part under 1 t therefore keeps the rotational inertia of a 1 t part.
  - Draining tanks rewrite the tensor every tick.
- The part's own mass is updated in one of two ways:
  - `UpdateMass()` for inactive parts: `prefabMass + Σ IPartMassModifier` (`Assembly-CSharp/Part.cs:4169`, `:4026-4058`).
  - `ModulesOnFixedUpdate` for staged (ACTIVE) parts (`Assembly-CSharp/Part.cs:5712-5749`).
  - Crew, crew-resource and inventory mass are added in both cases (`Assembly-CSharp/Part.cs:4171-4191`).
- `FlightIntegrator.UpdateMassStats` (`Assembly-CSharp/FlightIntegrator.cs:452-493`) then runs:
  - `resourceMass = Σ amount × density` (`Assembly-CSharp/Part.cs:6559-6572`).
  - `physicsMass = clamp(mass + resourceMass + physicslessChildMass, MinimumMass, …)`.
  - `rb.mass = max(MinimumRBMass, physicsMass)`. For robotic servos it is split half and half with `servoRb`.
  - `rb.centerOfMass = part.CoMOffset`.
  - The Rigidbody writes happen only if the vessel is not packed (`Assembly-CSharp/FlightIntegrator.cs:474-485`).
- Mass floors:
  - `partRBMassMin = 0.002` (`KSP-dev/Physics.cfg:136`).
  - `partMassMin` defaults to `Mathf.Epsilon` (`Assembly-CSharp/PhysicsGlobals.cs:407`), floored to 0.0001 by `AvailablePart` (`Assembly-CSharp/AvailablePart.cs:149-171`).

### 2.2 Physicsless parts (`PhysicsSignificance = 1`)

**Definition.**
- `enum PhysicalSignificance { FULL, NONE }` (`Assembly-CSharp/Part.cs:23-27`).
- The cfg field is `PhysicsSignificance` (default -1, `Assembly-CSharp/Part.cs:207`). It is converted in `Part.Start` (`Assembly-CSharp/Part.cs:1788-1805`).
- Exceptions to the conversion:
  - A part whose parent is robotic is promoted to FULL (`:1790-1797`).
  - The root part is always FULL (`:1830-1834`).
  - Docking-port parts are forced FULL (`Assembly-CSharp/ModuleDockingNode.cs:585-589`).
- 47 Squad part cfgs set it; struts and fuel lines are examples (`KSP-dev/GameData/Squad/Parts/CompoundParts/strutConnector/strutConnector.cfg:33`).

**Behaviour.** A NONE part has no Rigidbody and no attach joint (`Assembly-CSharp/Part.cs:1869`, `:1892-1895`). It stays a Transform child of its parent. `Part.Rigidbody` and `Part.RigidBodyPart` walk up to the first ancestor that has a body (`Assembly-CSharp/Part.cs:1049-1063`, `:1184-1198`).
- **Mass** folds into that ancestor through the recursive `GetPhysicslessChildMass` (`Assembly-CSharp/FlightIntegrator.cs:700-713`, added at `:470`). The child's mass *position* is ignored: the parent keeps its own `CoMOffset` (`:485`).
- **Forces** route to the ancestor: `Part.AddForce/AddForceAtPosition/AddTorque` accumulate on `RigidBodyPart` (`Assembly-CSharp/Part.cs:8663-8688`).
- **Drag** of a physicsless part is applied at the parent's `worldCenterOfMass` (`Assembly-CSharp/FlightIntegrator.cs:2243-2247`). The flags `ApplyDragToNonPhysicsParts` and `…AtParentCoM` default to true and are not in Physics.cfg (`Assembly-CSharp/PhysicsGlobals.cs:419-422`).

**Promotion and demotion.**
- `DemoteToPhysicslessPart` destroys the joint and the body; wheels keep their body (`Assembly-CSharp/Part.cs:2998-3023`).
- `PromoteToPhysicalPart` adds a body, copies the parent's velocity and rebuilds the descendants' joints (`Assembly-CSharp/Part.cs:2935-2976`).
- Decoupling a NONE part promotes it first (`Assembly-CSharp/Part.cs:2843-2846`).

### 2.3 PartJoint: how a ConfigurableJoint attachment is built

`PartJoint` is a MonoBehaviour on the host part that owns `List<ConfigurableJoint> joints` (`Assembly-CSharp/PartJoint.cs:4`, `:34`).

**Node choice.** The smaller of the two attach nodes is used (`Assembly-CSharp/PartJoint.cs:118-127`). If neither part has a node, it falls back to the parent's surface node (`:130-135`). The joint is rigid if either node is rigid (`:128`).

**Joint count per attachment.**
- **3** if node size ≥ 2 and neither part has a `ModuleDockingNode`. They sit 120° apart on a ring of radius `size−1` around the node axis (`Assembly-CSharp/PartJoint.cs:182-197`).
- **1** otherwise (`:198-210`).
- [runtime] PhysProf-S (21 parts) has 24 attach ConfigurableJoints: two size-2 stack attachments × 3, plus 18 × 1.

**Anchors and axes.** `anchor` and `connectedAnchor` come from the same world point, with `autoConfigureConnectedAnchor = false` and `configuredInWorldSpace = false` (`Assembly-CSharp/PartJoint.cs:279-311`).

**Stiffness** (`Assembly-CSharp/PartJoint.cs:293-303`; constants `:62-68`):
- Stack: `stiffness = 1500 × (size+1) × 2 × FlightGlobals.StackAttachStiffNess`.
- Surface: `1500 × (size+1) × 0.8 × SrfAttachStiffNess`.
- The two global multipliers are scene-serialized. **[runtime]** both are **10**, not the C# default of 1 (`Assembly-CSharp/FlightGlobals.cs:28-30`). So:
  - a size-1 stack joint has angular spring 60,000;
  - a size-2 stack joint has 90,000 per sub-joint. The measured Rockomax-to-probe joint has `angularXDrive.positionSpring = 90000`;
  - surface joints have 24,000.

**Limits** (`SetJointLimits`, `Assembly-CSharp/PartJoint.cs:359-395`):
- All six DOFs are `Limited`, or `Locked` if rigid.
- Angular limit 180°, or 0° if rigid. Linear limit 1 m. Limit springs and dampers are 0.

**Drives** (`Assembly-CSharp/PartJoint.cs:371-386`):
- Linear drive: `positionSpring = maximumForce = PhysicsGlobals.JointForce`, which defaults to 1e20 (`Assembly-CSharp/PhysicsGlobals.cs:374`) and is not read from Physics.cfg.
- Angular X and YZ drives: `positionSpring = stiffness` and `maximumForce = JointForce`, or ∞ if rigid.
- **Every drive damper is 0.** The only damping comes from the PhysX solver and Rigidbody drag. This is the source of KSP's "wobble".

**Not set.** `projectionMode`, `projectionDistance/Angle`, `enablePreprocessing`, `enableCollision` and the mass scales are never set on PartJoints, so Unity defaults apply. **[runtime]** values:
- `projectionMode = None` (projection distance 0.1 and angle 180, both unused)
- `enablePreprocessing = True`
- `enableCollision = False`
- `massScale = connectedMassScale = 1`
- measured `breakForce = breakTorque = 2200` kN on the probe-to-tank joint (22 × 6 / 3 × 50)

**Breaking.**
- At creation the joint stores `breakingForce = min(host, target).breakingForce × modifier(1) × (size+1)·factor / internalJoints` (torque likewise) (`Assembly-CSharp/PartJoint.cs:307-308`). Part defaults are 22 / 22 (`Assembly-CSharp/Part.cs:277-279`).
- The Unity joint itself is created **unbreakable** (+∞) (`:309-310`).
- A finite value is written only by `SetUnbreakable(false, rigid)`: `breakForce = breakingForce × JointBreakForceFactor × (rigid ? RigidJointBreakForceFactor : 1)` (`Assembly-CSharp/PartJoint.cs:338-342`). `jointBreakForceFactor = 50` (`KSP-dev/Physics.cfg:87-88`).
- Example: a size-1 stack joint between default parts breaks at 22 × 4 × 50 = 4400 kN.
- `SetUnbreakable(false)` is only called from `Part.ResetJoints` on Unpack, Couple and SetHierarchyRoot, unless the UnbreakableJoints cheat is on (`Assembly-CSharp/Part.cs:3811-3825`). `Pack` makes joints unbreakable again (`Assembly-CSharp/Part.cs:3779`).
- **Consequence: only `Part.attachJoint` can ever break.** Autostruts, struts, same-vessel docking joints and fairing payload joints keep +∞.

**Break detection.**
1. PhysX deletes the joint and sends `OnJointBreak(float)` to the host GameObject.
2. `PartJoint.OnJointBreak` handles it. It is ignored while `vessel.IgnoreCollisionsFrames > 0`. Otherwise it fires `onPartJointBreak`, calls `child.OnPartJointBreak`, and destroys all its joints and itself (`Assembly-CSharp/PartJoint.cs:397-417`).
3. `Part.OnPartJointBreak` → `decouple(breakForce)`, unless the part is the root (`Assembly-CSharp/Part.cs:3159-3169`).

The callback carries only the force, not which joint broke. Every PartJoint on that GameObject receives it, which is an inference from Unity's per-GameObject message dispatch.

### 2.4 Autostruts, struts, fuel lines, docking

**Autostruts** (`AutoStrutMode { Off, Root, Heaviest, Grandparent, ForceRoot, ForceHeaviest, ForceGrandparent }`, `Assembly-CSharp/Part.cs:72-81`).
- **Build:** `CycleAutoStrut` always destroys existing autostruts (`ReleaseAutoStruts`), then a coroutine waits one `WaitForFixedUpdate` and creates them (`Assembly-CSharp/Part.cs:8043-8091`, `:8217-8244`).
- **Joint shape:** one joint to the anchor, plus one to each of the anchor's symmetry counterparts. Each is a size-1, surface-type PartJoint (angular spring 1500 × 2 × 0.8 × 10 = 24,000 with the runtime stiffness multiplier), and unbreakable (`Assembly-CSharp/Part.cs:8098-8126`).
- **Anchor search:**
  - Heaviest uses `GetMassivePart`, a whole-tree walk that is O(n) per autostrutted part (`Assembly-CSharp/Part.cs:8342-8509`).
  - Grandparent is `parent.parent` (`:8320-8338`).
  - Root is the root part (`:8296-8311`).
- **Rebuild triggers:**
  - Part start (`Assembly-CSharp/Part.cs:1946`)
  - every `ResetJoints`, i.e. unpack, couple, re-root and rigid toggle (`:3824`)
  - every `onVesselWasModified`, `onPartCouple`, `onPartDie` or `onPartUndock` touching the vessel (`:1814-1817`, `:7214-7231`, `:8023-8041`)
  - `Vessel.CycleAllAutoStrut` (`Assembly-CSharp/Vessel.cs:4291-4306`)

**Struts (`CModuleStrut`)** create one surface-type PartJoint between the Rigidbody-owning ends (`Assembly-CSharp/CompoundParts/CModuleStrut.cs:26-53`).
- The cfg `linearStrength/angularStrength` (150) only reach `AttachNode.breakingForce`, which PartJoint never reads. Strut joints therefore stay unbreakable.
- The strut part itself is physicsless.

**Fuel lines (`CModuleFuelLine`)** create no joint. They only edit `fuelLookupTargets` (`Assembly-CSharp/CompoundParts/CModuleFuelLine.cs:12-32`).

**Docking two vessels** (`ModuleDockingNode.DockToVessel`, `Assembly-CSharp/ModuleDockingNode.cs:1424-1467`):
- The docking vessel is snapped into place, then `Part.Couple` (`Assembly-CSharp/Part.cs:3171-3212`) runs:
  - re-roots the docking vessel (every joint on the re-root path is recreated, `:3214-3255`)
  - `setParent` + `CreateAttachJoint`, which is **always 1 joint** because of the docking-port exclusion
  - `ResetJoints`, merges the part lists, `DestroyImmediate` on the old Vessel, rebuilds collision ignores, then `onVesselWasModified`
- **Undock:** `Part.Undock` destroys the joint, creates a new Vessel and rebuilds collision ignores (`Assembly-CSharp/Part.cs:3274-3353`). An ejection force (default 10) is split ±0.5 between the two sides (`Assembly-CSharp/ModuleDockingNode.cs:1517-1518`).
- **Same-vessel docking** adds `sameVesselDockJoint`. It is recreated every update if missing (`Assembly-CSharp/ModuleDockingNode.cs:956-961`, `:1703-1720`).

### 2.5 Decoupling

**`Part.decouple`** (`Assembly-CSharp/Part.cs:2804-2838`), by way of `Decoupling()` (`:2840-2886`):
1. Fires `onPartDeCouple`.
2. Promotes the part if physicsless, then `attachJoint.DestroyJoint()`.
3. `disconnect()` removes the subtree from `vessel.parts` (`:3084-3099`).
4. A new `Vessel` component is added and `Initialize()`d (`:2810-2812`). Its part walk is at `Assembly-CSharp/Vessel.cs:1776-1791`.
5. Collision ignores are rebuilt and `onVesselWasModified` fires for both vessels (`:2829-2833`).

**Ejection impulse.**
- `ModuleDecouple`:
  - Force is `ejectionForce × pct × 0.5`, split ±. `ejectionForce` defaults to 10 (`Assembly-CSharp/ModuleDecouplerBase.cs:9`).
  - It is applied after two rendered frames through `Part.AddForce`, i.e. for one physics tick as `ForceMode.Force` (`Assembly-CSharp/ModuleDecouple.cs:110-163`).
- `ModuleAnchoredDecoupler` applies `−force` to itself and `AddForceAtPosition` to the other side (`Assembly-CSharp/ModuleAnchoredDecoupler.cs:41-89`).

### 2.6 Same-vessel collision suppression

1. `Part.SetCollisionIgnores` only fires `OnCollisionIgnoreUpdate` (`Assembly-CSharp/Part.cs:2217-2223`). `CollisionManager` turns repeated events into one flag (`Assembly-CSharp/CollisionManager.cs:61-78`).
2. On the next FixedUpdate it rebuilds per-part collider lists for **every vessel in the scene** using `GetComponentsInChildren<Collider>`, which allocates (`Assembly-CSharp/CollisionManager.cs:80-124`).
3. It then calls `Physics.IgnoreCollision` on **every collider pair in the scene** whose Rigidbodies differ (`Assembly-CSharp/CollisionManager.cs:126-168`):
   - same vessel → ignore, unless both parts set `sameVesselCollision`;
   - different vessels → explicitly un-ignore.

**Cost:** O(C²) native calls. C is the number of scene colliders: for PhysProf-L, 446 parts give about 200k pairs. It runs after every part Start, unpack, couple, undock, decouple and promote, and after deployable or animation changes (e.g. `Assembly-CSharp/ModuleAnimateGeneric.cs:289`).

### 2.7 Vessel-level CoM, velocity, MOI

`VesselPrecalculate.CalculatePhysicsStats` (`Assembly-CSharp/VesselPrecalculate.cs:388-511`).

**Pass 1** (`:404-430`):
- `CoMD = Σ rb.worldCenterOfMass·physicsMass / Σm`
- mass-weighted velocity and angular velocity
- `velocityD = rb_velocityD + Krakensbane.FrameVel`

**Pass 2** (`:438-462`):
- Builds two TRS matrices per part to rotate `rb.inertiaTensor` into the vessel reference frame.
- Adds the parallel-axis term. This uses `rb.mass`/`rb.position`, not `physicsMass`/`worldCenterOfMass`.
- Keeps only the **diagonal** (`KSPUtil.Diag`, `Assembly-CSharp/KSPUtil.cs:1834-1837`).

**Cost:** about 10 Rigidbody/Transform interop reads per part per call.

**When it runs:**
- From `VesselPrecalculate.Update` once per new UT, i.e. once per rendered frame that advanced physics (`Assembly-CSharp/VesselPrecalculate.cs:207-224`).
- From `FixedUpdate` for the second and later ticks of a frame (`:172-199`).

---

## 3. Forces

### 3.1 Force accumulation and the single integration pass

**Buffers.** Each part carries three force buffers: `List<ForceHolder> forces`, `Vector3d force` and `Vector3d torque` (`Assembly-CSharp/Part.cs:839-843`, `:90-101`). Modules write into them through `Part.AddForce/AddForceAtPosition/AddTorque/AddImpulse` (`Assembly-CSharp/Part.cs:8663-8688`). These calls resolve to the nearest Rigidbody-owning ancestor.

**`FlightIntegrator.Integrate(partRef)`** recurses the attached tree from the root (`Assembly-CSharp/FlightIntegrator.cs:715-745`). For each part with a body it:
1. `rb.AddForce(precalc.integrationAccel, Acceleration)`, which is gravity
2. `rb.AddForce(part.force)`
3. `rb.AddTorque(part.torque)`
4. `rb.AddForceAtPosition` for every buffered `(force, pos)`
5. clears the buffers
6. calls `UpdateAerodynamics(part)`, which adds drag and body lift **directly** to the Rigidbody

Packed (on-rails) vessels just clear their buffers (`Assembly-CSharp/FlightIntegrator.cs:649-660`).

**Ordering hazard.** Forces buffered by a FixedUpdate that runs *after* `FlightIntegrator.FixedUpdate` in Unity's script order wait in the buffer until the next tick's Integrate. The measured order in §1 shows which modules are affected.

### 3.2 Gravity

- **Unity gravity is off for parts** (`Assembly-CSharp/Part.cs:1876`).
- **Gravity field:** `getGeeForceAtPosition = −r·gMagnitudeAtCenter/|r|³`, with `gMagnitudeAtCenter = GeeASL·g0·R²` and g0 = 9.80665 (`Assembly-CSharp/FlightGlobals.cs:1347-1354`, `Assembly-CSharp/CelestialBody.cs:539`, `Assembly-CSharp/PhysicsGlobals.cs:111`). `gravParameter = GeeASL·g0·R²`, computed through G = 6.67408e-11 (`Assembly-CSharp/CelestialBody.cs:540-541`).
- **Evaluated once per vessel,** at `vessel.CoMD`, in `VesselPrecalculate.CalculateGravity` (`Assembly-CSharp/VesselPrecalculate.cs:281-368`). Every part receives the same acceleration; there is no per-part or tidal term.
- **Rotating frame:** when the body is in inverse-rotation mode (below `inverseRotThresholdAltitude`, §5.4), Coriolis `−2ω×v` and centrifugal `ω×(ω×(c−p))` terms are added (`Assembly-CSharp/FlightGlobals.cs:1356-1373`, `Assembly-CSharp/VesselPrecalculate.cs:347-363`).
- **Physics easing:** after going off rails, `gravityMultiplier` ramps from 0 by 0.0625 per tick (`Assembly-CSharp/VesselPrecalculate.cs:48`, `:250-301`), gated by `PHYSICS_EASE = True` (`KSP-dev/settings.cfg:49`).
- **Orbit drift compensation** (unpacked, vacuum, `orbit.radius < 4e8`):
  - After 5 ticks within 1e-10 m² of the rails solution, gravity is replaced by a velocity offset that pins the vessel to its Keplerian path. It is applied before integration and corrected after (`Assembly-CSharp/VesselPrecalculate.cs:303-343`, `Assembly-CSharp/FlightIntegrator.cs:665-668`; thresholds `KSP-dev/Physics.cfg:95-97`).
  - This is why a coasting off-rails vessel does not drift from its orbit despite float precision.

### 3.3 Atmosphere state (per vessel per tick)

**Velocity.** `spd` is the magnitude of a **10-sample moving average** of vessel velocity, with spike rejection (`Assembly-CSharp/FlightIntegrator.cs:495-542`, `:607-614`).

**Atmosphere model.**
- Pressure: `CelestialBody.GetPressure` evaluates `atmospherePressureCurve`, or a lapse-rate formula (`Assembly-CSharp/CelestialBody.cs:387-402`).
- Density: `ρ = P·1000·M/(R·T)`, with R = 8.31447 (`Assembly-CSharp/CelestialBody.cs:430`, `Assembly-CSharp/PhysicsGlobals.cs:99`).
- Speed of sound: `√(γ·P/ρ)`, γ = 1.4 (`Assembly-CSharp/CelestialBody.cs:439`).

**Vessel-level outputs** (`CalculateConstantsAtmosphere`, `Assembly-CSharp/FlightIntegrator.cs:893-918`):
- `q = 0.0005·ρ·v²` (kPa)
- `mach`
- `pseudoReynolds = ρ·v`
- `pseudoReDragMult = DRAG_PSEUDOREYNOLDS(ρv)`
- the shock temperature

**Per-part outputs** (`Assembly-CSharp/FlightIntegrator.cs:2512-2543`):
- Static pressure is evaluated at each part's own altitude.
- `atmDensity` and `machNumber` are the vessel-wide values.

### 3.4 Drag: drag cubes

**Data.** A `DragCube` has six faces (±X, ±Y, ±Z), each with area, Cd and depth (`Assembly-CSharp/DragCube.cs:7-39`). Cubes are rendered offline by `DragCubeSystem`; procedural parts re-render at most once per second (`Assembly-CSharp/DragCubeSystem.cs:19`, `:617-649`).

**Setup.** `SetDragWeights` blends the weighted cubes, and `SetPartOcclusion` subtracts attached neighbours' facing areas (`Assembly-CSharp/DragCubeList.cs:234-343`). Both run only when flagged.

**`AddSurfaceDragDirection(localDir, mach)`** (`Assembly-CSharp/DragCubeList.cs:367-419`), per face i:
- `dotN = (dot(dir, nᵢ)+1)/2`
- `mult = lerp(Tail(M), Surface(M), 2·dotN)` if dotN ≤ 0.5, otherwise `lerp(Surface(M), Tip(M), 2·dotN−1)`; then `× DRAG_MULTIPLIER(M)` (`Assembly-CSharp/PhysicsGlobals.cs:2692-2705`)
- `aᵢ = areaOccludedᵢ·mult`
- `Cdᵢ = DRAG_CD(Cdᵢ)^DRAG_CD_POWER(M)` when Cdᵢ < 1
- `areaDrag += aᵢ·Cdᵢ`
- Body lift accumulates on windward faces: `−nᵢ·dot·area·Cd·BodyLift.lift(dot)`.

**Drag force** (`UpdateAerodynamics`, `Assembly-CSharp/FlightIntegrator.cs:2260-2389`):
- `v = part.Rigidbody.velocity + Krakensbane.FrameVel`. This is per part and unsmoothed (`:2282`).
- `F = q_part · (AreaDrag · DragCubeMultiplier(0.1)) · pseudoReDragMult · DragMultiplier(8) · (1 − submerged)` (`:2371-2374`; multipliers in `KSP-dev/Physics.cfg:129-130`).
- Applied with `rb.AddForceAtPosition(−v̂·F, CoP)` as `ForceMode.Force` (`:2243-2247`; `DragUsesAcceleration = false`, `Assembly-CSharp/PhysicsGlobals.cs:527`).

**Angular drag** uses PhysX: `rb.angularDrag = part.angularDrag(2) · q_atm · AngularDragMultiplier(2)` (`Assembly-CSharp/FlightIntegrator.cs:2359-2365`).

**Body lift:** `q · bodyLiftMultiplier(18) · BodyLift.liftMach(M) · rotate(DragCubes.LiftForce)`, projected perpendicular to v (`Assembly-CSharp/FlightIntegrator.cs:2249-2257`, `:2381-2383`).

**Curves** (`KSP-dev/Physics.cfg:137-275`): DRAG_TIP, DRAG_SURFACE, DRAG_TAIL, DRAG_MULTIPLIER, DRAG_CD, DRAG_CD_POWER, DRAG_PSEUDOREYNOLDS and the BodyLift curves.
- All are `FloatCurve`, i.e. Unity `AnimationCurve` Hermite keys (`Assembly-CSharp/FloatCurve.cs:8`, `:54-76`).
- **A port needs a bit-compatible Hermite evaluator, including Unity's auto-tangent rule for 2-value keys.**

**Shielding:** `ModuleCargoBay` uses `Physics.OverlapSphere` plus `Collider.Raycast` to find enclosed parts, but only on events (`Assembly-CSharp/ModuleCargoBay.cs:374-531`). Shielded parts get no drag.

### 3.5 Lifting surfaces

`ModuleLiftingSurface` runs in its own Unity `FixedUpdate` (`Assembly-CSharp/ModuleLiftingSurface.cs:170`). It reads `part.dynamicPressurekPa` as computed by FlightIntegrator.

| Quantity | Formula | Citation |
|---|---|---|
| Velocity | `GetPointVelocity + FrameVel` | `Assembly-CSharp/ModuleLiftingSurface.cs:180` |
| Q | `q·1000`, i.e. Pa | `:183-184` |
| Lift | `sign·liftCurve(|dot|)·liftMachCurve(M)·deflectionLiftCoeff·Q·LiftMultiplier(0.036)` | `:239-251` |
| Drag | `dragCurve·dragMachCurve·deflectionLiftCoeff·Q·LiftDragMultiplier(0.015)` | `:269-274` |

- Both forces are **buffered** through `part.AddForceAtPosition` (`:188-193`).
- `ModuleControlSurface` overrides `FixedUpdate` (`Assembly-CSharp/ModuleControlSurface.cs:366-441`):
  - It splits the surface into a fixed part (1 − ctrlSurfaceArea) and a deflected part.
  - Deflection follows `MoveTowards(deflection, input·range, actuatorSpeed·dt)` (`:704-743`).

### 3.6 Buoyancy and water

`PartBuoyancy`, one Unity `FixedUpdate` per physical part (`Assembly-CSharp/PartBuoyancy.cs:231`).

| Quantity | Formula | Citation |
|---|---|---|
| Displaced volume | from the drag-cube box | `Assembly-CSharp/PartBuoyancy.cs:171-229` |
| Depth | 8 transformed cube corners | `:259-366` |
| Submerged fraction | sine-remapped | `:497-514` |
| Force | `−g(CoB)·V·submerged·ρ_ocean·BuoyancyScalar(1.2)·part.buoyancy`, buffered at the centre of buoyancy | `:533-534`, `:643`; `KSP-dev/Physics.cfg:58` |

**Water drag goes through PhysX linear `rb.drag`:** `0.5·ρ·|v|·AreaDrag·0.1·scalar·submerged·DragMultiplier/rb.mass` (`Assembly-CSharp/PartBuoyancy.cs:549-593`).

### 3.7 Engines

`ModuleEngines.FixedUpdate` is a Unity callback. `OnFixedUpdate` is not used. The tick runs throttle → propellant → thrust → FX (`Assembly-CSharp/ModuleEngines.cs:1587-1602`). It is skipped on rails (`:1530-1538`).

**Throttle response** is a lerp at 0.2/0.1·dt when `useEngineResponseTime` is set (`Assembly-CSharp/ModuleEngines.cs:1433-1446`).

**Flow and Isp:**
- `maxFuelFlow = maxThrust/(Isp_vac·g0)` (`:639`).
- `realIsp = atmosphereCurve(p_atm)`.
- `massFlow = maxFuelFlow·throttle·ModifyFlow(atmCurve(ρ/1.225)·velCurve(M)·thrustCurve)` (`:1454-1476`, `:2055-2101`).
- `thrust = massFlow·Isp·g0·fraction_received` (`:1473-1511`).

**Propellant request:** `part.RequestResource` → `Vessel.resourcePartSet` or crossfeed sets (`Assembly-CSharp/ModuleEngines.cs:581`, `Assembly-CSharp/Part.cs:6480-6540`).

**Force:** `part.AddForceAtPosition(−thrustTransform.forward·thrust·multiplier, transform.position)` per thrust transform, buffered (`Assembly-CSharp/ModuleEngines.cs:1551-1556`).

**Per engine per tick:**
- One `Physics.Raycast` for exhaust damage (10 m range) (`:1314-1341`).
- Heat: `heatProduction·thrust/maxThrust·0.025·thermalMass` (`:1560-1566`).

### 3.8 RCS, reaction wheels, SAS

**RCS** (`ModuleRCS`):
- Input is sampled in `Update` at frame rate (`Assembly-CSharp/ModuleRCS.cs:211-262`).
- Per-nozzle throttle is `max(0, n·(ω_cmd × r̂)) + max(0, n·v_cmd)`, clamped (`:321-347`).
- Force is `thrusterPower·f·Isp(p)/Isp_vac·fraction`, buffered at the nozzle (`:357`, `:566-611`).

**Reaction wheels** (`ModuleReactionWheel.FixedUpdate`):
- Input is the stick `(pitch, roll, yaw)`, scaled by torque (default 5) and authority, then lagged by `Lerp(.., 30·dt)` (`Assembly-CSharp/ModuleReactionWheel.cs:137-207`).
- Output is `part.AddTorque(world)`, buffered.

**SAS** (`VesselAutopilot.VesselSAS`) only writes `FlightCtrlState` pitch/roll/yaw through three PIDs (`Assembly-CSharp/VesselAutopilot.cs:790-826`). The PIDs run inside `FlightInputHandler.FixedUpdate` → `Vessel.FeedInputFeed` (`Assembly-CSharp/FlightInputHandler.cs:672-691`, `Assembly-CSharp/Vessel.cs:2035-2040`). **SAS never applies torque itself.**

---

## 4. Thermal

`FlightIntegrator` is a `VesselModule` (MonoBehaviour) that runs only on loaded vessels (`Assembly-CSharp/FlightIntegrator.cs:5`, `:230-233`). Thermal happens in two places.

### 4.1 Per physics tick (`FlightIntegrator.FixedUpdate`, `Assembly-CSharp/FlightIntegrator.cs:544-670`)

1. `UpdateMassStats`. Every tick, the thermal mass is computed as `C = m·800·thermalMassModifier + Σ resource·cp`; the skin share is `C_skin = clamp(0.001·skinMassPerArea·A_rad·800, …, 0.5·C)`; and the internal remainder is `C_int` (`:452-463`, `:2236-2241`).
2. `CalculateConstantsAtmosphere/Vacuum`. This produces the vessel convective coefficient h (`:893-961`):
   - Newtonian: `h_N = ρ^0.5·(8.14 + v)·4`
   - Mach: `h_M = 7e-7·ρ^0.5·v³`
   - `h` is a lerp between them, weighted by `clamp01((M−2)/3)³`.
   - Shock temperature: `T_shock = lerp(v, 21·v^0.75, machLerp)`.
3. `DragCubeSetupAndPartAeroStats`. Three O(n) loops (`:2512-2543`).
4. `CheckThermalGraph`. This rebuilds the conduction graph if dirty and always calls `UpdateCompoundParts` (`:963-972`).
5. `UpdateThermodynamics` (`:974-1135`), which runs **before the packed early return**. Logic:
   - It skips unless 0.014 s of game time has accumulated.
   - It switches to **analytic mode** when warp > 100× or after more than 4 s unloaded (`:995`).
   - **Numeric mode:** per-part precalc, then one forward-Euler pass at 1×, or Heun pairs at higher warp. The pass count is `round(((dt/2)^0.75)·10)`, so 100× warp gives 10 pairs (`:1103-1132`; `KSP-dev/Physics.cfg:20`).
   - **Inside each pass** (`ThermalIntegrationPass`, `:1137-1306`):
     - `UpdateConduction()` once for the vessel. It sorts parts by temperature (O(n log n), only when unsorted) and covers internal, skin-skin and skin-internal conduction (`:1457-1847`). The scale is `ConductionFactor(120)·10·k·contactArea` with a mean-temperature clamp.
     - Then per part: convection `(T_post − T_skin)·h·A_conv·0.001·mult`, capped at `C_skin·f` (`:2090-2120`), and radiation `σ·ε·A·(T⁴ − T_bg⁴)` (`:2182-2212`).
     - Conduction and convection are multiplied by `ThermalConvergenceFactor = 0.63`; radiation is not (`:1220-1253`).
6. `Integrate` (§3.1). Its `UpdateAerodynamics` refreshes the exposed areas that the **next** tick's thermal pass will use. Thermal therefore lags aero by one tick.

### 4.2 Per rendered frame (`FlightIntegrator.Update`, `:672-693`)

- `CalculateSunBodyFlux`: one `Physics.Raycast` in scaled space for sun visibility, then solar flux `L/(4πd²)` (`:798-835`).
- Occlusion, throttled to every 0.039 s of game time and round-robin across convection, solar and body (`:687-692`, `:2545-2569`):
  - **Convection occlusion** (only when M > 1) sorts parts along the velocity vector. It builds shock cones whose angles depend on M and part fineness (`Assembly-CSharp/OcclusionCone.cs:60-120`) and tests each part against all upstream cones. This is **O(n·k), worst case O(n²)**, with an early exit (`Assembly-CSharp/FlightIntegrator.cs:2572-2668`).
  - Solar and body occlusion use the same sort plus rectangle-overlap tests (`:2670-2769`).
  - This path calls `FXCamera.Instance.ApplyObliqueness` — a rendering side effect inside the occlusion code (`:2636`).

### 4.3 Overheat

- `Part.FixedUpdate → CheckPartTemp`: if `T > maxTemp` (default 2000) or the skin temperature exceeds its limit, the part explodes with probability `tempExplodeChance(0.8)·dt·ratio` per tick. This is **stochastic** and uses `UnityEngine.Random` (`Assembly-CSharp/Part.cs:4196`, `:4213-4226`).
- Pressure and g-force limits use the same pattern (`Assembly-CSharp/Part.cs:4228-4258`).

**Constants that differ between Physics.cfg and the code defaults (cfg wins).**

| Constant | Physics.cfg | Code default |
|---|---|---|
| conductionFactor | 120 | 20 |
| skinSkinConductionFactor | 0.003 | 0.005 |
| skinInternalConductionFactor | 0.005 | 0.03 |
| machConvectionFactor | 7 | 6 |
| thermalIntegrationHighMaxPasses | 10 | 20 |

Sources: `KSP-dev/Physics.cfg:20,41,49-51` and `Assembly-CSharp/PhysicsGlobals.cs:177-258`.

---

## 5. Precision, frames, rails and the physics bubble

### 5.1 Krakensbane: velocity frame

- **Threshold:** `MaxV = 5` m/s, so `MaxVSqr = 25` (`Assembly-CSharp/Krakensbane.cs:7`, `:39`). **[runtime]** 5 m/s confirmed.
- **Engages** when the active vessel is unpacked, `SafeToEngage` holds, and `|velocityD|² > 25` (`:55-85`). `SafeToEngage` requires:
  - not landed or splashed
  - radar altitude ≥ 150 m alone, or 300 m with other loaded vessels
  - no other loaded vessel landed or low (`:88-119`)
- **`AddExcess`** moves the whole rigidbody-frame velocity of the active vessel into `FrameVel` (`:121-147`). It then:
  - calls `vessel.ChangeWorldVelocity(−excess)` on every unpacked loaded vessel, which writes `rb.velocity` on every part (`Assembly-CSharp/Vessel.cs:3197-3217`);
  - applies `AddForce(−excess, VelocityChange)` to every physical object.
- **While engaged** it feeds `FrameVel·dt` to `FloatingOrigin.SetOutOfFrameOffset` every tick (`Assembly-CSharp/Krakensbane.cs:81-84`).
- **Net effect:** above about 5 m/s, PhysX rigidbodies of the active vessel move at near-zero velocity, and the world (bodies, terrain, other vessels) is moved past them.
- **Consumers:**
  - All aero code adds `GetFrameVelocity()` back (`Assembly-CSharp/FlightIntegrator.cs:2282`, `Assembly-CSharp/ModuleLiftingSurface.cs:180`).
  - `velocityD = rb_velocityD + FrameVel` (`Assembly-CSharp/VesselPrecalculate.cs:425`).

### 5.2 FloatingOrigin: position frame

- **Thresholds:** `threshold = 500` m and `velForContinuous = 5000` m/s (`Assembly-CSharp/FloatingOrigin.cs:6`, `:10`). **[runtime]** both confirmed.
- **`FixedUpdate`** (`:125-163`) shifts when Krakensbane allows it and either:
  - the active vessel is more than 500 m from the origin, or
  - continuous mode is on (rails warp above the physics rate, or more than 5 km/s).

  **Whenever Krakensbane has a non-zero frame velocity, it shifts every tick** (`:146-150`).
- **`setOffset`** (`:166-290`) moves:
  - every CelestialBody, writing `bodyTransform.position` and `pqsController.PrecisePosition`;
  - every vessel, including unloaded ones, via `SetPosition`, which writes `partTransform.position` for every unpacked part (`Assembly-CSharp/Vessel.cs:3685-3710`);
  - **every live world-space particle** of every part FX group, with `GetParticles`/`SetParticles` (`:207-244`);
  - FXMonger, registered particle systems, physical objects and the terrain shader offset.
- **Cost per shift:** O(bodies + Σ vessel parts + live particles). During an ascent it runs every tick (measured in PROFILE.md).

### 5.3 Double-precision orbit ↔ float rigidbody synchronisation

**OrbitDriver modes:** `TRACK_Phys` (off rails), `UPDATE` (on rails) and `IDLE` (`Assembly-CSharp/OrbitDriver.cs:6-11`).

**Off rails, every tick: physics → orbit.**
- `TrackRigidbody` sets `pos = (CoMD − body.position).xzy` and `vel = velocityD.xzy + rotFrameVel` (`Assembly-CSharp/OrbitDriver.cs:221-249`).
- `orbit.UpdateFromStateVectors(pos, vel, body, UT)` then recomputes the full Keplerian element set (`Assembly-CSharp/Orbit.cs:448-539`).
- So the authoritative state of an off-rails vessel is the float PhysX state plus the double `FrameVel` and the floating-origin offset. The orbit is *derived* from it every tick.

**On rails: orbit → transform.**
- `updateFromParameters` → `orbit.UpdateFromUT` (Kepler solve) → `vessel.SetPosition(body.position + pos − rot·localCoM)` (`Assembly-CSharp/OrbitDriver.cs:256-299`).
- Solver (`Assembly-CSharp/Orbit.cs:890-957`):
  - e < 0.8: Newton to 1e-7, with no iteration cap
  - 0.8 ≤ e < 1: 8 fixed Laguerre-type iterations
  - hyperbolic: Newton

**Axes:**
- The orbit frame is Z-up and Unity is Y-up. `xzy` and `Swizzle()` swap y and z (`Assembly-CSharp/Vector3d.cs:14`, `:90-95`).
- `QuaternionD.swizzle = (−x, −z, −y, w)` (`Assembly-CSharp/UnityEngine/QuaternionD.cs:56`).
- `Vector3d` → `Vector3` conversion truncates to float (`Assembly-CSharp/Vector3d.cs:97-105`).

**UT:** `Planetarium.FixedUpdate` advances UT by `fixedDeltaTime·timeScale`, using its own constant 0.02 (`Assembly-CSharp/Planetarium.cs:82`, `:201-212`).

### 5.4 Rotating frame

- The C# default `inverseRotThresholdAltitude` is 15000 m, clamped up to `atmosphereDepth` (`Assembly-CSharp/CelestialBody.cs:239-243`, `:372-375`). **[runtime]** Kerbin's scene value is **100,000 m** (its atmosphere depth is 70,000 m). So on Kerbin the whole atmospheric flight regime, and low orbit below 100 km, runs in the co-rotating frame.
- Below the threshold, `OrbitPhysicsManager.checkReferenceFrame` sets `inverseRotation`. The body's transform then stops rotating and the *planetarium* rotates instead (`Assembly-CSharp/OrbitPhysicsManager.cs:84-105`, `:238-300`, `Assembly-CSharp/CelestialBody.cs:553-565`).
- Every unpacked part's `rb.velocity` is shifted by ∓ω×r at the switch.
- Below the threshold, physics runs in a co-rotating frame; that is why Coriolis and centrifugal terms are added (§3.2).

### 5.5 Packed vs loaded; load and pack distances

| Flag | Meaning | Citation |
|---|---|---|
| `loaded` | part GameObjects exist | `Assembly-CSharp/Vessel.cs:111` |
| `packed` | on rails, kinematic bodies | `Assembly-CSharp/Vessel.cs:123` |

**`GoOnRails`** (`Assembly-CSharp/Vessel.cs:2356-2393`):
- `Part.Pack` sets `rb.isKinematic = true`, disables buoyancy and makes joints unbreakable. **Bodies and joints are kept** (`Assembly-CSharp/Part.cs:3747-3782`).
- The OrbitDriver switches to `UPDATE`, or `IDLE` when landed.

**`GoOffRails`** (`Assembly-CSharp/Vessel.cs:2395-2488`):
- `Part.Unpack` sets `isKinematic = false`, then `ResetJoints`, which restores breakability and rebuilds autostruts. Collision ignores are rebuilt (`Assembly-CSharp/Part.cs:3784-3825`).
- `ResumeVelocity` sets each part's velocity from the orbit (`Assembly-CSharp/Part.cs:3832`).
- Mode becomes `TRACK_Phys`, and Krakensbane engages for the active vessel.

**Transitions:**
- **Load/unload:** `Vessel.Update`, using float `Vector3.Distance` to the active vessel (`Assembly-CSharp/Vessel.cs:3013-3034`).
- **Pack/unpack:** `OrbitPhysicsManager.LateUpdate` (`Assembly-CSharp/OrbitPhysicsManager.cs:107-153`). In rails warp above the physics rate, everything packs.

**Default `VesselRanges`** (`Assembly-CSharp/VesselRanges.cs:66-78`; identical in `KSP-dev/Physics.cfg:334-383`):

| Situation | load | unload | pack | unpack |
|---|---|---|---|---|
| prelaunch / landed / splashed / orbit / escaping | 2250 | 2500 | 350 | 200 |
| flying | 2250 | 22500 | 25000 | 2000 |
| subOrbital | 2250 | 15000 | 10000 | 200 |

So "loaded within 2.25 km" means *full part physics* only for FLYING vessels. A landed or orbiting neighbour within 2.25 km is loaded (GameObjects and kinematic bodies exist) but stays packed until it is within 200 m.

**Unloaded vessels** still run `VesselPrecalculate` every tick: a Kepler solve, `SetPosition`, `UpdatePosVel` and `CheckKill` (`Assembly-CSharp/VesselPrecalculate.cs:156-203`).

---

## 6. Time warp

- **Modes:** `HIGH` (rails) and `LOW` (physics) (`Assembly-CSharp/TimeWarp.cs:8-12`).
- **Rate assignment** (`TimeWarp.updateRate`, `Assembly-CSharp/TimeWarp.cs:419-433`):
  - `Planetarium.TimeScale = r`
  - `Time.timeScale = (LOW ? r : 1)`
  - `Time.fixedDeltaTime = 0.02·Time.timeScale`
- **Physics warp:** `timeScale` = r, so ticks happen about 50 times per real second but each step lasts 0.02·r s of game time. Joint stiffness is fixed, so larger steps destabilise joints, which is why physics warp is capped at 4× **[runtime]**.
- **Rails warp:** `timeScale` = 1 and `fixedDeltaTime` = 0.02. `TimeWarp.fixedDeltaTime` returns `0.02·rate`, and UT advances `0.02·rate` per tick (`Assembly-CSharp/TimeWarp.cs:174-188`). Everything is packed and propagated with Kepler.
- **Rate arrays** are scene-serialized. **[runtime]** values:
  - `warpRates = 1, 5, 10, 50, 100, 1000, 10000, 100000`
  - `physicsWarpRates = 1, 2, 3, 4`
  - `altitudeLimits = 0, 0.05, 0.05, 0.1, 0.2, 0.4, 0.8, 1` (× body radius)
  - `maxPhysicsRate_index = 0`, so `MaxPhysicsRate = warpRates[0] = 1`: any rails warp above 1× packs every vessel. Per-body limits are `body.timeWarpAltitudeLimits` (`Assembly-CSharp/CelestialBody.cs:499-503`, `:1012-1022`).
- **Rails warp is blocked** by:
  - static pressure > 0
  - acceleration above 0.1 g
  - an EVA kerbal on a ladder
  - being landed and moving
  - altitude limits, with a one-tick look-ahead

  It is clamped near SOI changes (`Assembly-CSharp/TimeWarp.cs:508-627`).
- **Max physics Δt per frame:** `PHYSICS_FRAME_DT_LIMIT = 0.04` (`KSP-dev/settings.cfg:39`; default `Assembly-CSharp/GameSettings.cs:995`) → `Time.maximumDeltaTime` (`Assembly-CSharp/GameSettings.cs:2016`).
  - At 0.02 s per tick this allows **at most 2 physics ticks per rendered frame**.
  - When a frame takes longer than 40 ms, game time runs slower than wall time, the "yellow clock". PROFILE.md measures this as `sim_rate_ut_per_wall`.

---

## 7. Wheels, legs and terrain collision

### 7.1 Wheels are a hybrid of PhysX WheelCollider and Edy's Vehicle Physics

**One controller per wheel part.** `ModuleWheelBase.wheelSetup` adds one `KSPWheelController : VehicleBase` per wheel part, with one wheel and its own solver (`Assembly-CSharp/ModuleWheelBase.cs:280-312`, `Assembly-CSharp/KSPWheelController.cs:174-213`).

**`VPWheelCollider` wraps a real `UnityEngine.WheelCollider`** (`Assembly-CSharp/VehiclePhysics/VPWheelCollider.cs:397-416`):
- The Unity tire friction is zeroed.
- The wheel mass is set to 1.
- One PhysX substep is requested.
- PhysX supplies the **suspension raycast and spring/damper force**. VPP supplies the **tire forces and drive**.

**Per tick, `VehicleBase.FixedUpdate`** (`Assembly-CSharp/VehiclePhysics/VehicleBase.cs:934-1033`):
1. Reads `WheelHit`.
2. Computes load and contact frame, using `GetPointVelocity`.
3. Runs the drive/brake blocks.
4. Runs an explicit-Euler `Solver.Integrate` with 8 substeps for the active vessel and 4 otherwise (`Assembly-CSharp/GameSettings.cs:1061-1062`, `Assembly-CSharp/VehiclePhysics/Solver.cs:181-207`).
5. Applies the smoothed tire force with `AddForceAtPosition(force, hit.point)`, plus a reaction on the other rigidbody.

**Tire friction curve:** three points (adherent, peak, limit), defaults 0.25 / 1.45 / 1.1 × `frictionMultiplier` (`Assembly-CSharp/ModuleWheelBase.cs:55-71`, `:376-409`). Force is `curve(|slip|)·load·grip` (`Assembly-CSharp/VehiclePhysics/TireFriction.cs:437-450`). Whether the Smooth or the Lineal curve is in effect is undetermined (§12).

**Suspension auto-tuning** happens every grounded tick (`ModuleWheelSuspension.SuspensionSpringUpdate`, `Assembly-CSharp/ModuleWheels/ModuleWheelSuspension.cs:355-419`):
- spring = `springRatio·sprungMassGravity·boost`
- damper = `√(spring·damperRatio·fudge)`
- hysteresis before writing to `WheelCollider.suspensionSpring` (`Assembly-CSharp/VehiclePhysics/VPWheelCollider.cs:414-473`)

**Other module ticks:**
- Motor torque: `torqueCurve(|ωr|)·input`, electric-charge limited (`Assembly-CSharp/ModuleWheels/ModuleWheelMotor.cs:131-238`).
- Brakes (`Assembly-CSharp/ModuleWheels/ModuleWheelBrakes.cs:88-104`).
- Ackermann steering, updated at **frame rate** (`Assembly-CSharp/KSPWheelController.cs:230-238`).
- Damage stress model (`Assembly-CSharp/ModuleWheels/ModuleWheelDamage.cs:289-366`).

**Wheel side effects on the part Rigidbody:**
- `rb.maxAngularVelocity = 14` (`Assembly-CSharp/VehiclePhysics/VehicleBase.cs:906`).
- `maxDepenetrationVelocity` is lerped (`:936-944`).
- When fully braked and still, a `RigidbodyConstraints.FreezeAll` "schloompa" lock is applied (`Assembly-CSharp/ModuleWheelBase.cs:602-621`).

**Per frame:** one visual `Physics.Raycast` per wheel (`Assembly-CSharp/VehiclePhysics/VPWheelCollider.cs:255-257`).

### 7.2 Landing legs

Landing legs are wheels: `ModuleWheelBase wheelType = LEG` with Suspension, Deployment, Lock, Bogey and Damage submodules (`KSP-dev/GameData/Squad/Parts/Utility/landingLegLT-1/landingLegLT-1.cfg:100-191`).
- `ModuleLandingLeg` no longer exists; only a save upgrader mentions it (`Assembly-CSharp/SaveUpgradePipeline/v110_WheelModuleOverhaul.cs:29`).
- Legs use distributed sprung mass from the `SuspensionLoadBalancer` VesselModule (`Assembly-CSharp/SuspensionLoadBalancer.cs:31-59`).
- `ModuleWheelLock` gives them a permanent 1000 N·m brake (`Assembly-CSharp/ModuleWheels/ModuleWheelLock.cs:8-13`).
- While retracted, the WheelCollider is destroyed and a `SphereCollider` stand-in is used (`Assembly-CSharp/ModuleWheels/ModuleWheelDeployment.cs:355-403`).

### 7.3 Terrain colliders

- **Only `PQSMod_QuadMeshColliders` creates them** (`Assembly-CSharp/PQSMod_QuadMeshColliders.cs:17-44`). Each quad at subdivision ≥ `maxLevel − |maxLevelOffset|` gets a non-convex `MeshCollider` whose `sharedMesh` is the quad mesh. It is assigned in `OnQuadBuilt` and released in `OnQuadDestroy`.
- **Quad subdivision** follows the PQS target (the active vessel) inside the per-frame `PQS.UpdateSphere` coroutine (`Assembly-CSharp/PQS.cs:1104-1127`, `:1411-1440`). Thresholds are at `Assembly-CSharp/PQS.cs:749-755` and `Assembly-CSharp/PQ.cs:280-346`. The Kerbin Default preset is `maxSubdivision 9` (`KSP-dev/settings.cfg:2102-2108`).
- **Layers** (from the binary `globalgamemanagers`): 15 Local Scenery is terrain and scatter; 0 is parts; 26/27 are wheel colliders. KSP makes no `Physics.IgnoreLayerCollision` call, so the layer matrix is project data.
- **KSC buildings** are activated by distance in `PQSCity`. They take damage on impact via `DestructibleBuilding.OnCollisionEnter` (`Assembly-CSharp/DestructibleBuilding.cs:606-693`).

### 7.4 Contact, landed state, anti-tunnelling

**`Part.OnCollisionEnter`** (`Assembly-CSharp/Part.cs:3355-3415`):
- marks ground contact on layer 15
- calls `HandleCollision`, which explodes the part if `|v_rel| > crashTolerance` (default 9) and, against a rigidbody, `|v_rel|·m_other > crashTolerance` (`:3613-3655`)

**`Part.OnCollisionStay`** only re-runs Enter for 3 frames after a collision reset (`Assembly-CSharp/Part.cs:3941-3969`). Defining it at all makes PhysX report contact-stay callbacks every step for every touching part.

**`CollisionEnhancer.FixedUpdate`** runs on every physical part (`Assembly-CSharp/CollisionEnhancer.cs:47-184`):
- If the part moved more than 0.1 m since the last tick, one `Physics.Linecast` against layer 15.
- On a hit it explodes, or teleports to `hit + 0.25·up` and reflects the velocity × 0.2.
- During a Krakensbane ascent parts barely move in world space, so the linecast is mostly skipped. When landed it is skipped too.

**Landed state:**
- `Vessel.checkLanded` is an O(parts) scan (`Assembly-CSharp/Vessel.cs:1015-1042`).
- `updateSituation` runs in `LateUpdate` (`:3133-3188`).
- A height raycast runs per `UpdatePosVel` (`Assembly-CSharp/Vessel.cs:2652-2673`).
- `CheckKill` destroys a vessel that falls below the terrain (`:2768-2783`).

**Landed anchor:** a still, landed vessel without wheels gets a `FixedJoint` "RB anchor" to the world after 1.5 s at v² < 0.05 (`Assembly-CSharp/Vessel.cs:1105-1166`, `:3097`). [runtime] It is the 25th joint in PhysProf-S on the pad.

---

## 8. Unity-coupling map

| Class / subsystem | Inseparable from Unity | Pure math (ports as-is) |
|---|---|---|
| `Part` | Rigidbody (create, mass, `inertiaTensor`, `isKinematic`, `maxAngularVelocity`), coroutine `Start`, `FixedUpdate/Update/LateUpdate`, `OnCollisionEnter/Stay/Exit`, Transform hierarchy (physicsless children are Transform children), `UnityEngine.Random` for overheat | mass and resource sums, overheat/pressure/g limits, force buffers (`Part.cs:8663-8688`) |
| `PartJoint` | ConfigurableJoint, JointDrive, SoftJointLimit, `breakForce` + `OnJointBreak` message, `Object.Destroy` | stiffness and break formulas (`PartJoint.cs:293-308`, `:338-342`), node choice |
| `CollisionManager` | `Physics.IgnoreCollision`, `GetComponentsInChildren<Collider>` | the pairing policy |
| `FlightIntegrator` | `rb.AddForce/AddForceAtPosition/AddTorque`, `rb.angularDrag`, `TransformPoint/InverseTransformDirection`, `Physics.Raycast` (sun), `FXCamera` side effect | atmosphere, drag-cube evaluation, thermal (conduction/convection/radiation/analytic), shock-cone occlusion geometry (needs part bounds) |
| `DragCubeList` / `DragCubeSystem` | offline cube *rendering* uses cameras | all runtime cube math |
| `FloatCurve` | wraps `AnimationCurve` | needs a bit-compatible Hermite evaluator |
| `VesselPrecalculate` | reads `rb.worldCenterOfMass/velocity/angularVelocity/inertiaTensor`, `Physics.SyncTransforms` | gravity, Coriolis, drift compensation, MOI sums |
| `ModuleEngines` / `ModuleRCS` / `ModuleReactionWheel` / `ModuleLiftingSurface` | `Transform.forward/position`, `Physics.Raycast` (exhaust), `GetPointVelocity`, Unity `FixedUpdate`/`Update` | thrust/Isp/flow, RCS allocation, wheel torque lag, lift/drag curves |
| `PartBuoyancy` | `rb.drag` (PhysX linear damping), `MultiplyPoint3x4` | volume, submersion, force |
| `Krakensbane` | `rb.velocity` writes over all parts, `AddForce(VelocityChange)` | `FrameVel` bookkeeping |
| `FloatingOrigin` | `Transform.position` writes for everything, ParticleSystem buffers, `Shader.SetGlobalVector` | none; it exists only because of float transforms |
| `OrbitDriver` / `Orbit` / `Planetarium` | `driverTransform` writes, MonoBehaviour lifecycle | Kepler solve, element ↔ state vectors, SOI search, UT |
| `Vector3d` / `QuaternionD` | implicit float conversion only | everything |
| `TimeWarp` | `Time.timeScale`, `Time.fixedDeltaTime`, `Time.maximumDeltaTime` | rate-limit rules |
| Wheels (`ModuleWheel*`, `VehiclePhysics`) | `WheelCollider` (suspension raycast and force inside PhysX), `GetPointVelocity`, `AddForceAtPosition`, `RigidbodyConstraints`, layers | tire curves, VPP wheel/solver/drive blocks, suspension tuning, damage model |
| PQS terrain | `MeshCollider` on quad meshes, `GameObject.SetActive`, coroutine | subdivision rules, height evaluation (PQSMods, LibNoise) |
| `CollisionEnhancer` | `Physics.Linecast`, `rb.velocity`/`position` writes | decision logic |

---

## 9. Quirks found in the code (decide per item whether to reproduce)

These are behaviours a straight port would reproduce by accident. Each needs an explicit keep-or-fix decision.

**Joints and structure**

1. **Only attach joints can break** (§2.3). Autostruts, EAS-4 struts, same-vessel docking joints and fairing payload joints are effectively unbreakable. The strut `linearStrength`/`angularStrength` cfg values never reach PhysX (`Assembly-CSharp/CompoundParts/CModuleStrut.cs:38-39`).
2. **Inertia floor for light parts.** The inertia tensor is scaled by `max(1, mass)`, so parts under 1 t keep the rotational inertia of a 1 t part (`Assembly-CSharp/Part.cs:4263-4268`). This is a deliberate stability hack, and it changes the attitude dynamics of small craft.
3. **Diagonal-only vessel MOI.** Vessel MOI drops the products of inertia (`Assembly-CSharp/VesselPrecalculate.cs:462`). SAS tuning uses it (`Assembly-CSharp/VesselAutopilot.cs:500-505`).
4. **Zero joint damping.** All joint dampers are 0 (§2.3). Wobble is damped only by the PhysX solver: 50 position iterations [runtime], with no projection.

**Gravity and aerodynamics**

5. **Uniform gravity.** Gravity is evaluated once per vessel at the CoM, so long vessels feel no gravity gradient (§3.2).
6. **Inertial airspeed above the rotating frame.** Aero velocity is rigidbody velocity plus Krakensbane frame velocity. There is no wind, and above the inverse-rotation threshold (100 km on Kerbin [runtime], i.e. above the atmosphere) it is inertial rather than surface-relative.
7. **Mixed-time aero inputs.** Wing lift uses last tick's dynamic pressure, and thermal uses last tick's exposed area (§1.3).
8. **Unity curve evaluation.** `FloatCurve` is Unity `AnimationCurve`. Two-value keys get Unity auto-tangents, which are not documented in the C# (`Assembly-CSharp/FloatCurve.cs:54-59`).

**Thermal**

9. **Solar attenuation compounds.** `solarFlux *= solarFluxMultiplier` runs every FixedUpdate, but `solarFlux` is only reset in Update (`Assembly-CSharp/FlightIntegrator.cs:815`, `:833`, `:896`). The attenuation therefore compounds when a frame has two ticks.
10. **Inter-part skin conduction uses assignment.** It writes `= 0.0 - x` where `-=` was intended, so each link overwrites rather than accumulates (`Assembly-CSharp/FlightIntegrator.cs:1768` vs `:1535`).
11. **Integer division in ModuleCoreHeat.** Radiator distribution truncates to 0 or 1 (`Assembly-CSharp/ModuleCoreHeat.cs:459`, `:475`, `:484`).
12. **Convergence factor only partly applied.** `ThermalConvergenceFactor = 0.63` scales conduction and convection but not radiation or injected flux (`Assembly-CSharp/FlightIntegrator.cs:1220-1253`).
13. **The shock "Mach angle" is `asin(1/√M)`**, not `asin(1/M)` (`Assembly-CSharp/FlightIntegrator.cs:2620-2621`).

**Failure, input and frame mechanics**

14. **Stochastic destruction.** Overheat, overpressure and over-G use `UnityEngine.Random` per tick (`Assembly-CSharp/Part.cs:4213-4258`), so runs are not bit-reproducible.
15. **Frame-rate-dependent inputs.** RCS input and wheel steering are sampled in `Update`, at frame rate (`Assembly-CSharp/ModuleRCS.cs:211-262`, `Assembly-CSharp/KSPWheelController.cs:230-238`). Occlusion is throttled by game time but read per frame.
16. **Rendering call inside physics.** The occlusion pass calls `FXCamera.Instance.ApplyObliqueness` (`Assembly-CSharp/FlightIntegrator.cs:2636`).
17. **Every-tick origin shift.** While Krakensbane holds a non-zero frame velocity, FloatingOrigin shifts the whole world every physics tick (`Assembly-CSharp/FloatingOrigin.cs:146-150`). This is a cost, not a behaviour (§11).
18. **Collision ignores rebuilt from scratch.** Every structural change rebuilds the ignore pairs for all colliders in the scene (§2.6).
19. **Spawn collapse without struts.** [observed] The 88 m, 457-part test stack without autostruts collapsed within about one second of unpacking on the pad. The same craft with `Grandparent` autostruts on every part flew (PROFILE.md). The joint model needs struts to hold tall heavy stacks. That is part of KSP's gameplay, but not the Kraken-style self-destruction.

## 10. What "faithful" means

The reimplementation should reproduce what a player can observe and plan around. It should not reproduce PhysX internals.

### Must preserve (observable, tested by players)

**Orbital mechanics**
- Kepler orbits with patched-conic SOI transitions (§5.3).
- Exact on-rails propagation and time-warp rate tables [runtime]: 1, 5, 10, 50, 100, 1000, 10000, 100000 on rails; 1-4× physics warp.
- Warp limits: altitude, atmosphere, acceleration (§6).
- Orbit stability while coasting off rails. KSP achieves this with drift compensation (§3.2); any approach that keeps a coasting orbit fixed is acceptable.

**Forces**
- Uniform gravity at the CoM, with Coriolis and centrifugal terms in the rotating frame below the threshold (§3.2, §5.4).
- Engine thrust, Isp and flow curves.
- RCS allocation, reaction-wheel torque and lag, SAS behaviour (PID on stick inputs).
- Drag-cube aerodynamics: tip/surface/tail curves, Mach, pseudo-Reynolds, body lift.
- Lifting-surface curves.
- Buoyancy.
- The 0.8× effective drag scaling (`DragMultiplier 8 × DragCubeMultiplier 0.1`).
- Terminal velocities and ascent profiles to within a few percent. This is the gameplay-visible check: ΔV maps and ascent guides depend on it.

**Thermal**
- Re-entry heating magnitudes and timing.
- Part overheat thresholds.
- The visible "which part burns first" ordering, which comes from shock-cone occlusion.
- Skin vs internal temperature split, radiators, ablators.
- The analytic warp catch-up.

**Mass and structure**
- Resource mass drain feeding back into the vessel.
- CoM shift as tanks drain; physicsless parts folding into their parent's mass.
- Staging and decoupling: ejection impulses, new vessel creation, debris that keeps flying.
- Docking and undocking.
- Structural failure under extreme load. Something must break when a player overloads a joint, e.g. a 600 t payload on one size-1 node, or high-q aero loads on a wide unstrutted stack. The threshold rule (`breakingForce × (size+1)·factor / joints × 50`) should be kept as the *capacity* model, even if it is evaluated as a stress check rather than a PhysX break.

**Contact and wheels**
- Landing: crash tolerance by impact speed, landing-leg suspension, wheel traction and steering, braking.
- Building and terrain impacts.

**Physics bubble**
- 2.25 km load range; pack/unpack ranges per situation (§5.5).
- Vessels in other situations stay on rails.

### May be dropped or changed (not gameplay, or widely considered a defect)

- **Joint wobble and noodling.** The visual oscillation of tall stacks comes from undamped soft ConfigurableJoints. Struts and autostruts would still exist as *strength* modifiers.
- **Kraken failure modes:** spontaneous explosions from solver instability, depenetration launches (`maxDepenetrationVelocity = 1e32` [runtime]), and the spawn collapse in quirk 19.
- **Float-precision artefacts:** jitter far from origin, Krakensbane, and FloatingOrigin every-tick shifts.
- **Stochastic destruction timing** (quirk 14). Deterministic thresholds are acceptable.
- **The per-part inertia floor** (quirk 2) and the diagonal-only MOI (quirk 3). Correct these if SAS is retuned to match.
- **Unbreakable struts and autostruts** (quirk 1). Making them breakable is a defensible correction; keeping them unbreakable is also acceptable.
- **The thermal bugs** (quirks 9-13). Reproduce them only if heating numbers must match the stock game exactly.
- **Frame-rate-coupled input sampling** (quirk 15).

## 11. Performance improvement opportunities, ranked

All costs below were measured on the reference machine (Ryzen AI 7 350) with the profiling build; see [PROFILE.md](PROFILE.md). "Tick" is one 0.02 s physics step.

**The heaviest case is L2-AS** (457 parts, 960 joints, 49 engines, powered ascent):
- 15.8 ms of CPU per tick, 38 ms per frame.
- 1.74 ticks per frame, 26 FPS.
- Game time runs at 0.91× real time.

**Ranking** is by measured share of the heaviest tick, with exact changes ahead of approximate ones at similar share.

**Gain classes** apply to the physics tick unless stated otherwise:
- **10x**: removes ≥ 90% of the targeted cost.
- **2x**: removes 40-90%.
- **marginal**: removes < 40%, or the targeted cost is small.

| # | Change | Behaviour | Targets (L2-AS ascent, ms/tick) | Gain class |
|---|---|---|---|---|
| 1 | No per-tick world shifting: double-precision state, camera-relative rendering, vessel-local physics frame | exact | FloatingOrigin 3.3 + extra PhysX on shift ticks ~2.25 = **5.6 (35%)** | **2x** on ascent ticks (10x on the targeted cost) |
| 2 | Data-oriented per-part pipeline (SoA aero, thermal, mass, limits, buoyancy, anti-tunnel; no per-part MonoBehaviours) | exact | FI 3.3 + Part 0.7 + Buoyancy 0.7 + Enhancer 0.4 + precalc stats ≈ **5.5 (35%)** | **10x** on that code; ~1.5x of whole tick |
| 3 | Reduced rigid-body vessel model (rigid clusters + structural stress graph) | approximate (wobble gone; breaking becomes a stress check) | PhysX 1.9 baseline + per-part body upkeep ~1.5 (~21%); grows with joint count | **10x** on PhysX and joint work; largest remaining cost after #1 and #2 |
| 4 | Batch PartModule ticks (engines, RCS, wings, wheels) into systems; precomputed resource-flow sets; batched raycasts | exact | module FixedUpdates 1.0 + SAS input 0.4 | 2x on those; marginal overall |
| 5 | Multithread per-vessel and per-part loops (after #2) | exact (with deterministic partitioning) | everything in #2, and loaded neighbour vessels | 2x-4x on part loops; marginal for one vessel after #3 |
| 6 | Do not run per-part physics for non-active loaded vessels (rigid single body, or keep on rails until contact or proximity) | approximate | not measured (single-vessel test); scales with debris and neighbours | 2x-10x in debris-heavy scenes (unmeasured) |
| 7 | Incremental collision filtering (collision groups) instead of an all-pairs `IgnoreCollision` rebuild | exact | not in steady state; O(C²) spike per structural event | removes multi-ms spikes (unmeasured) |
| 8 | Allocation/GC elimination | exact | about 1 MB/s; one GC per 20-60 s, costing about 10-20 ms of hitch | marginal (hitch removal only) |
| 9 | Replace the PhysX soft-ConfigurableJoint chain with a custom articulated or XPBD/Jolt solver while keeping one body per part | approximate | PhysX baseline 2.0 | 2x on PhysX; superseded by #3 |
| 10 | Skip aero for parts and vessels with q ≈ 0; skip buoyancy work for vessels far from water | exact | about 1.2 FI aero + 0.7 buoyancy on the pad | marginal; 2x on the pad and in vacuum |

### 11.1 No per-tick world shifting (rank 1)

**Cost mechanism.**
- Above 5 m/s, Krakensbane moves the active vessel's velocity into `FrameVel` (`Assembly-CSharp/Krakensbane.cs:121-147`).
- While `FrameVel ≠ 0`, `FloatingOrigin` shifts **the whole world every tick** (`Assembly-CSharp/FloatingOrigin.cs:146-150`). Each shift moves:
  - all celestial bodies, which carry the PQS terrain and its MeshColliders
  - all vessels' part transforms
  - every live world-space particle of every part (`GetParticles`/`SetParticles`)
  - physical objects (`Assembly-CSharp/FloatingOrigin.cs:166-290`)
- Measured script cost of `FloatingOrigin.FixedUpdate` per ascent tick: S 0.35 ms, M2 1.24 ms, L2-AS 3.32 ms. It scales with parts and engine particle systems.
- **PhysX cost per tick doubles on shift ticks.** In the same L2-AS ascent window:
  - ticks with a shift: 3.8-4.1 ms
  - ticks without: 1.9-2.25 ms (PROFILE.md §4)
- After a shift PhysX has to re-sync every moved collider. The scene census at 457 parts counts 1,325 static colliders (917 static MeshColliders, mostly terrain) plus 457 dynamic bodies.
- Pad→ascent PhysX for S grows from 0.49 to 1.83 ms/tick with only 21 bodies. So most of the ascent PhysX cost is this floating-origin effect, not the rocket.

**Proposed change.**
- Keep vessel, part and body state in double precision in a body-centred (or SOI-centred) frame, which the Orbit code already uses (§5.3).
- Run the local physics solver in a frame attached to the active vessel. Its origin is moved only rarely, for example every few km, by translating the *solver's* bodies rather than the static world.
- Generate terrain collision for the bubble in that local frame.
- Render with camera-relative transforms computed from doubles each frame. Rendering never feeds back into physics.

**Behaviour.** Exact. Floating origin and Krakensbane are numerical bookkeeping, and their observable side effects are precision artefacts that may be dropped (§10).

**Gain.** About 35% of the L2-AS ascent tick (5.6 of 15.8 ms) and about 43% of the M2 ascent tick (1.24 ms script + 1.56 ms extra PhysX of 6.45 ms) → **2x** on ascent ticks.

### 11.2 Data-oriented per-part pipeline (rank 2)

**Cost mechanism.** Per part per tick, everything runs as Mono-JIT managed code with Unity interop calls. Measured at 457 parts, pad (`L2-AS`):

| Per-part work | µs per part per tick |
|---|---|
| FlightIntegrator total | 7.3 |
|   UpdateMassStats | 2.2 |
|   UpdateAerodynamics | 2.5, even on the pad at q ≈ 0 |
|   thermal | 1.4 |
|   Integrate force flush | 0.9 |
| `Part.FixedUpdate` (`ValidateInertiaTensor` interop reads, mass update, crew, heat gauge, temperature/pressure/g limit checks; `Assembly-CSharp/Part.cs:4092-4211`) | 1.5 |
| `PartBuoyancy.FixedUpdate`, on dry land | 1.5 |
| `CollisionEnhancer.FixedUpdate` | 1.0 |
| `VesselPrecalculate.CalculatePhysicsStats`, about 10 Rigidbody/Transform reads per part, per frame, plus again in FixedUpdate on 2-tick frames | ~0.7 |

- That is about 6 ms per tick at 457 parts, scaling linearly with part count.
- Three Unity callbacks per part per tick (Part, PartBuoyancy, CollisionEnhancer) are dispatched natively before any useful work is done.

**Proposed change.**
- Struct-of-arrays part state in the simulation core: mass, resources, drag-cube areas and Cd per face, thermal state, positions in cluster space.
- Tight loops for mass/CoM/MOI, drag-cube aero, thermal conduction/convection/radiation and limit checks.
- One system update per vessel, not per-part callbacks.
- The formulas stay identical (§3, §4).

**Behaviour.** Exact, apart from floating-point summation order.

**Gain.** These are simple arithmetic loops: about 50-200 flops per part per stage. Native SoA code typically runs at 0.05-0.3 µs per part per stage, against 1-2.5 µs measured. That is **10x** on about 35% of the tick. This is an estimate: no native prototype was benchmarked.

### 11.3 Reduced rigid-body vessel model (rank 3)

**Cost mechanism.**
- Every FULL part is a PhysX body (`Assembly-CSharp/Part.cs:1868-1875`).
- Every attachment is 1-3 soft `ConfigurableJoint`s (`Assembly-CSharp/PartJoint.cs:182-210`). Autostruts add about one more per part (§2.4).
- PhysX solves all of them with **50 position iterations** [runtime], against Unity's default of 6.
- Measured PhysX time per tick, on the pad with no floating-origin shifts:

  | Craft | Bodies | Joints | PhysX ms/tick |
  |---|---|---|---|
  | S | 21 | 24 | 0.49 |
  | M2 | 153 | 168 | 0.77 |
  | L2-AS | 457 | 960 | 1.96 |
  | L2-AS ascent, ticks without a shift | 457 | 960 | 1.9-2.25 |

- Every part body also drags per-part interop work into the tick:
  - `rb.mass`, `centerOfMass` and `inertiaTensor` writes (`Assembly-CSharp/FlightIntegrator.cs:470-485`, `Assembly-CSharp/Part.cs:4261-4273`)
  - force flushes (`Assembly-CSharp/FlightIntegrator.cs:715-745`)
  - CoM/MOI reads (`Assembly-CSharp/VesselPrecalculate.cs:404-462`)

**Proposed change.**
- Weld every run of parts between articulation points into one rigid body with summed mass, CoM and inertia. Articulation points are decouplers and separators (until fired), docking ports, robotic hinges and servos, wheel and leg suspensions, and parts the player marks "flexible".
- Forces from all welded parts are accumulated onto the cluster body at their world positions, exactly as KSP already does for physicsless parts (§2.2), so the force code stays as it is.
- Keep a **structural graph**: the original attach nodes with KSP's capacity formula (`breakingForce·(size+1)·factor/joints·50`, §2.3).
- Each tick, compute internal joint loads from the cluster's acceleration and the external forces per part: a static equilibrium solve over the tree, O(n) for trees, with a small sparse solve where struts create loops. Split the cluster when a load exceeds capacity.
- Struts and autostruts become stiffness and strength edges of that graph.

**Behaviour.** Approximate.
- Preserved: mass properties, forces, breaking under overload, and decoupling.
- Lost: wobble, oscillation-driven failures and "noodle" bending.
- The 457-part stack would be 1 body on the pad and in flight, or 49 if boosters are kept as separate clusters, against 457 bodies and 960 joints today.

**Gain.**
- PhysX and joint cost: **10x or more**. Fewer bodies and constraints, and no 50-iteration chain solve.
- Whole tick: about 1.3x on its own; about 2x of what remains after #1 and #2.
- This is the change the feasibility study calls "the 10x lever". The measurement supports it for the PhysX and joint part. But PhysX is only 13-25% of today's KSP tick (§11 table). It ranks third on measured share, and it becomes the **largest remaining cost once #1 and #2 are done**.

It is also the only item that does more than save time:
- It removes the instability class behind quirk 19.
- It allows physics warp beyond 4×: with a stiff joint chain, a larger dt is the stability limit, and a rigid cluster has no such limit.

### 11.4 Module systems, SAS and resource flow (rank 4)

**Cost mechanism.**
- `ModuleEngines.FixedUpdate` costs about 20 µs per engine per tick: 0.97 ms for 49 engines in the L2-AS ascent. That covers throttle, `RequestPropellant` through the vessel's resource sets (`Assembly-CSharp/ModuleEngines.cs:581`), a `Physics.Raycast` for exhaust damage (`:1314`) and FX.
- `FlightInputHandler.FixedUpdate` (SAS PIDs and the autopilot torque survey) grows from 0.06 ms (21 parts) to 0.44 ms (457 parts) per tick.

**Proposed change.**
- Engines as a system with precomputed propellant source sets, invalidated on staging or docking.
- Exhaust impingement as one batched ray query per tick.
- SAS torque capacity cached per vessel and updated on part-set changes.

**Behaviour.** Exact.

**Gain.** 2x on about 9% of the tick → marginal overall.

### 11.5 Multithreading (rank 5)

**Current state.** All KSP physics scripts run on the main thread; only PhysX itself is internally threaded. FlightIntegrator and VesselPrecalculate are per-vessel MonoBehaviours with no cross-vessel data except the global frame (Krakensbane, origin).

**Proposed change.** Run per-vessel pipelines as parallel jobs. Within a vessel, run the aero and per-part thermal loops in parallel; conduction is a graph and needs edge colouring or a two-phase gather.

**Behaviour.** Exact, with deterministic reduction order.

**Gain.** On the 16-thread machine, 2-4x on the part loops, but those are already small after #2. The large win is with many loaded vessels (#6).

### 11.6 Non-active vessel part physics (rank 6)

**Cost mechanism.**
- Every loaded, unpacked vessel runs the full per-part pipeline: FlightIntegrator, per-part callbacks and PhysX bodies (§5.5).
- Flying debris unpacks within 2 km and packs only beyond 25 km. Debris from staging therefore stays fully simulated for a long time.
- Not measured: the test flew a single vessel.

**Proposed change.** Simulate non-active loaded vessels as single rigid bodies, with drag from summed cubes and no per-part thermal unless re-entering, until they come within contact range of something.

**Behaviour.** Approximate. Debris tumbling detail changes; trajectories stay.

**Gain.** Proportional to debris count. 2-10x in staging-heavy scenes, unmeasured.

### 11.7 Collision filtering (rank 7)

**Cost mechanism.** `CollisionManager.UpdatePartCollisionIgnores` calls `Physics.IgnoreCollision` for **every collider pair in the scene** after each structural event (`Assembly-CSharp/CollisionManager.cs:126-168`). About 105k pairs at 458 colliders. Not measured: no staging event fell inside a window.

**Proposed change.** Per-vessel collision groups (filter masks) in the solver.

**Behaviour.** Exact.

**Gain.** Removes O(C²) spikes on staging, docking and decoupling.

### 11.8 Allocation and GC (rank 8)

**Measured.**
- Steady-state allocation is about 0.3-1.3 MB/s (lower bound).
- 0-3 full collections per minute.
- Frames containing a collection take 13-25 ms, against 4-14 ms median (PROFILE.md §5).
- During the run-1 L self-destruction (explosions and debris) allocation rose to 20 MB/s, with 14 GC/min.

**Assessment.** GC is a periodic hitch, **not** a throughput cost, in the steady state measured here.

**Proposed change.** A native core with arena or pool allocation removes it.

**Gain class.** Marginal for average FPS; it removes the hitch.

### 11.9 Joint solver replacement while keeping per-part bodies (rank 9)

**Proposed change.** If per-part bodies must stay (to keep wobble), replace PhysX `ConfigurableJoint` with:
- a reduced-coordinate articulation (Featherstone; PhysX/Jolt/MuJoCo-style) for the tree, plus loop-closure constraints for struts; or
- XPBD with substeps.

Either avoids 50 global iterations.

**Behaviour.** Approximate: different wobble character.

**Gain.** 2x-5x on the PhysX share (about 2 ms/tick at 457 parts). Superseded by #3.

### 11.10 Cheap exact skips (rank 10)

- `UpdateAerodynamics` runs on every part even when `q ≈ 0` (pad, vacuum). It costs 1.1 ms/tick at 457 parts on the pad.
- `PartBuoyancy` runs per part on dry land: 0.67 ms/tick.
- Skipping both behind a vessel-level test is **exact**. It removes about 30% of the pad tick at 457 parts.

### 11.11 Outside physics, for context

These do not belong to the physics engine but compete for the same frame:
- **PQS terrain** (`PQS.UpdateQuads` coroutine): 0.55-0.72 ms per frame, independent of the craft. That is **14-20% of the frame for the 21-part craft**.
- **Rendering on the integrated GPU at low settings:** 0.9-2.7 ms per frame of CPU-side submission. GPU wait is negligible (PROFILE.md §8).
- **KSP UI scripts:** 0.3-0.5 ms per frame.

At 21 parts these, not physics, dominate the frame.

## 12. Unverified items

**Engine internals.**
- PhysX internals are not observable from C#. The split of PhysX time between joint solving, broadphase and static-collider re-sync after origin shifts is inferred from correlations, not measured inside PhysX.
- The Unity layer collision matrix and tag setup are project data and were not decoded.
- `WheelCollider` internal force computation is PhysX vehicle-SDK code.

**Behaviour not exercised by the test runs.**
- Joint-break and decouple paths: no break or staging separation happened inside a measurement window; the run-1 self-destruction was not profiled in isolation.
- Docking.
- Wheels and legs at runtime: no wheeled craft was flown.
- Water, buoyancy forces and thermal re-entry heating: the test ascents peaked at 384 m/s, so there was no hypersonic convection or shock-cone occlusion load.
- Time warp at more than 1×: physics warp was not used.
- Any vessel other than the active one.

**Values not fully resolved.**
- Whether `TireFriction` uses the Smooth or the Lineal curve at runtime (§7.1).
- Per-body `maxLevelOffset` for terrain colliders.

**Scope and method limits.**
- All gain estimates for proposed changes are engineering estimates. Nothing was prototyped.
- Measurements come from one machine (Ryzen AI 7 350, integrated GPU). Absolute times differ elsewhere; the shares should transfer to similar desktop CPUs.
- The profiled DLL is the decompiled rebuild compiled with `/optimize+`, not the shipped binary. The original build flags are unknown, so absolute times may differ slightly from stock KSP.

