# PROGRESS: wheel / landing-leg spike (branch `wheelspike`, from main `b9119103`)

Task: opt-in prototype `core/spikes/wheels/` + assessment `core/WHEEL-SPIKE.md` (what the C# does, Jolt strategy,
prototype on real craft, measured cost, comparison, WP-29 package split).

## Log

- 2026-10-12: read optimizer.md, CLAUDE.md, PHYSICS-SPIKE.md, spikes/jolt structure, STATUS WP-20a/WP-19a, NEXT-STEPS session 4,
  work_packages.csv WP-29 rows (163 files). Next: read the decompiled wheel code.
- 2026-10-12: read ModuleWheelBase, KSPWheelController, VPWheelCollider, VehicleBase, VPP Wheel/Solver/Block/DirectDrive,
  TireFriction, ModuleWheelSuspension/Motor/Brakes/Steering/MotorSteering/Lock/Deployment/Damage, SuspensionLoadBalancer,
  KSPWheelGravity, GameSettings WHEEL_*. Findings so far:
  - `tireFriction.model = Lineal` (ModuleWheelBase.cs:361) never rebuilds `m_curve` (only the ctor / OnAfterDeserialize call
    SetupFrictionCurves): runtime curve is SmoothFriction (cubic) by the C# reading. Needs a capture to confirm.
  - Native: PhysX 4.1 suspension law (PxVehicleUpdate.cpp, fetched): F = max(0, sprungMass*max(0,g.w) + k*jounce) + c*jounceSpeed,
    projected on the hit normal; KSP's Physics.gravity = 0 [runtime] -> sprung-mass term vanishes -> F = max(0, k (c - target D)) + c dc/dt.
    Unity docs: targetPosition 0 = full extension (matches VPP's use). Hard limit at full compression (constraint, not in hit.force).
  - M1 wheel: rescaleFactor 1.25 default -> collider radius 0.4725 but the motor / slip use the cfg 0.378 (traction-control quirk).
  - Static deflection of KSP's auto-tuned suspension = g0 / springRatio (independent of gravity) + target offset.
- 2026-10-12/13: built `core/spikes/wheels/` (wheel_model = managed port, rover = reduced body + two native variants
  (explicit PhysX law / Jolt VehicleConstraint + null friction controller), scenarios + analytic references, main CLI).
  Fixed: SPH craft frame (AssembleVesselFromCraft re-roots: up = inverse(root file rot) * Y), dangling TireFriction pointer,
  leg geometry uses the "Deployed" drag cube. Rest / drive / touchdown traces sane on bug-e, prospector, twostage.
  Next: run the bench sets (rest, drive, brake, touchdown, cost), write WHEEL-SPIKE.md.
