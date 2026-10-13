# PROGRESS: wheel / landing-leg spike (branch `wheelspike`, from main `b9119103`)

Task: opt-in prototype `core/spikes/wheels/` + assessment `core/WHEEL-SPIKE.md` (what the C# does, Jolt strategy,
prototype on real craft, measured cost, comparison, WP-29 package split).

State: DONE. Assessment in `core/WHEEL-SPIKE.md`, results in `core/spikes/wheels/results/`, reruns with
`bash spikes/wheels/tools/run_benches.sh build/spike-jolt` (20 s).

## Log

- 2026-10-12: read optimizer.md, CLAUDE.md, PHYSICS-SPIKE.md, spikes/jolt structure, STATUS WP-20a/WP-19a, NEXT-STEPS session 4,
  work_packages.csv WP-29 rows (163 files).
- 2026-10-12: read ModuleWheelBase, KSPWheelController, VPWheelCollider, VehicleBase, VPP Wheel/Solver/Block/DirectDrive,
  TireFriction, ModuleWheelSuspension/Motor/Brakes/Steering/MotorSteering/Lock/Deployment/Damage, SuspensionLoadBalancer,
  KSPWheelGravity, GameSettings WHEEL_*. Findings:
  - `tireFriction.model = Lineal` (ModuleWheelBase.cs:361) never rebuilds `m_curve` (only the ctor / OnAfterDeserialize call
    SetupFrictionCurves): the runtime curve is SmoothFriction by the C# reading. Needs a capture to confirm.
  - Native: PhysX 4.1 suspension law (PxVehicleUpdate.cpp, fetched): F = max(0, sprungMass*max(0,g.w) + k*jounce) + c*jounceSpeed,
    projected on the hit normal; KSP's Physics.gravity = 0 [runtime] -> sprung-mass term vanishes -> F = max(0, k (c - target D)) + c dc/dt.
    Unity docs: targetPosition 0 = full extension (matches VPP's use). Hard limit at full compression (constraint, not in hit.force).
  - M1 wheel: rescaleFactor 1.25 default -> collider radius 0.4725 but the motor / slip use the cfg 0.378 (traction-control quirk).
  - Auto-tuned spring k proportional to the per-wheel load in g-tonnes: deflection g0 / (springRatio tweak boost), plus the
    PhysX rest offset c0 = target D.
- 2026-10-12/13: built `core/spikes/wheels/` (wheel_model = managed port, rover = reduced body + two native variants
  (explicit PhysX law / Jolt VehicleConstraint + null friction controller), scenarios + analytic references, main CLI).
  Fixed: SPH craft frame (AssembleVesselFromCraft re-roots: up = inverse(root file rot) * Y), dangling TireFriction pointer,
  leg geometry uses the "Deployed" drag cube. Commit 69dcbc51 (WIP).
- 2026-10-13: benches. Found: VPP wheel inertia floor (I >= 0.01) dominates small rovers (reference needs m + sum I/r^2);
  torque rounding sets the S2 top speed (10.72 m/s); explicit suspension unstable at 3x (lander) / 4x (rovers); VPP brake/tyre
  limit cycle from 2x (both natives); 100 % brakes lock at 1000 kN m (stoppie / flip from 20 m/s); touchdown 3 m/s: no bounce-launch,
  50 % travel explicit vs 62 % Jolt VC; cost 0.6-0.9 us per wheel per tick, rover tick 6-16 us. Deterministic (two runs identical).
  WP-29 reachable subset: 8,867 of 29,217 lines (34 files); the rest of Edy's VPP / EdyCommonTools is unreachable.
- 2026-10-13: wrote WHEEL-SPIKE.md (recommendation, measurements, risks, WP-29a-e split with gates). Full suite rerun (default
  configuration, spikes off) to confirm nothing changed.
