# PROGRESS WP-52b: joint drive backend for the servos on the reduced Jolt model

Branch `wp52b` (worktree `.worktrees/wp52b`, from main 51530d89; main e43c8576 merged as 445d842f). Optimizer (Opus).

## Design (decided 2026-10-20)

- PhysicsWorld: `CreateArticulatedConstraint(a, b, desc)` = Jolt HingeConstraint / SliderConstraint whose reference is the PHYSICAL zero of
  the servo coordinate; `SetArticulatedDrive` each step = Jolt motor `PositionAndVelocity`, `StiffnessAndDamping` spring (k, c), torque /
  force limit = maximumForce; limits + limit spring (soft when spring > 0). Jolt's soft-constraint motor equals the plant's implicit drive law
  when the target velocity is 0 (hinge, piston) and to O(k dt vt) for the rotor (k = 1e-10). `GetConstraintImpulse` returns world impulses
  (hinge rotation lambdas rebuilt from the pre-step rotations). Per-joint solver iterations (Jolt override), 50 for servo islands.
- VesselPhysics: the servo body (Part.servoRb) is a mass element of its own (FlightIntegrator.cs:476-480: rb.mass = servoRb.mass = num2 / 2f)
  in the cluster of the moving side; the servo joint is the constraint between the part's cluster and the servo body's cluster. Hooks in
  VesselPhysicsOptions: servoBody, servoBodyEnd, servoDrive, servoMoved, servoVelocitySteps. Loads: the servo body is a node, the servo joint an
  external force on both nodes (no element, no capacity).
- Serenity: ServoDriveLaw (one mapping, shared by HeadlessServoPlant and the backend), UseServoBackend(opts).

## Steps

1. [x] PhysicsWorld articulated constraints + world-level drive-law tests (Python reference): 1.6e-5 / 4.9e-5 relative
2. [x] VesselPhysics servo bodies (split, mass, drag, loads, hooks)
3. [x] Serenity backend + FI partHasServoRb
4. [x] Scenario gates (hinge, piston, rotor, locked, bubble), determinism 1 / 4 threads; commit 47cea203
5. [x] Docs (STATUS WP-52b + row 59, README rows, progress.csv WP-52 0.85), merge main e43c8576
6. [x] Fresh full verification: ctest 49/49, unit 1033 cases / 1,599,617 assertions in default, rs=11, rs=27 orders; STATUS filled

## Findings so far

- GS convergence: hinge / piston vs plant 3.6e-3 rad / 13 mm at 10 iterations, 4.6e-4 / 4.3 mm at 50, 3e-5 / 0.5 mm at 200 -> servo islands 50.
- Velocity-level solver dissipates (1 - cos w dt) of off-axis tangential speed per step (characterised); rotor scenario uses mainAxis Y.
- Light anchored base (72:1) converges badly for fast rotors; documented.
- Bench: servo joint ~ +0.01 ms/tick over a weld (S head 0.0085 ms, L2-AS 0.129 ms); 50 iterations within noise.
