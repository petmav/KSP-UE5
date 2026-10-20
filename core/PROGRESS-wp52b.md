# PROGRESS WP-52b: joint drive backend for the servos on the reduced Jolt model

Branch `wp52b` (worktree `.worktrees/wp52b`, from main 51530d89). Optimizer (Opus).

## Design (decided 2026-10-20)

- PhysicsWorld: `CreateArticulatedConstraint(a, b, desc)` = Jolt HingeConstraint / SliderConstraint whose reference is the PHYSICAL zero of
  the servo coordinate; `SetArticulatedDrive` each step = Jolt motor `PositionAndVelocity`, `StiffnessAndDamping` spring (k, c), torque /
  force limit = maximumForce; limits + limit spring (soft when spring > 0). Jolt's soft-constraint motor equals the plant's implicit drive law
  when the target velocity is 0 (hinge, piston) and to O(k dt vt) for the rotor (k = 1e-10). `GetConstraintImpulse` returns world impulses
  (hinge rotation lambdas rebuilt from the pre-step rotations).
- VesselPhysics: the servo body (Part.servoRb) is a mass element of its own (FlightIntegrator.cs:476-480: rb.mass = servoRb.mass = num2 / 2)
  in the cluster of the moving side; the servo joint is the constraint between the part's cluster and the servo body's cluster. Hooks in
  VesselPhysicsOptions: servoBody (geometry), servoBodyEnd (which end of an attach joint is the servo body), servoDrive (per step), servoMoved
  (after the step). Loads: the servo body is a node of the load model, the servo joint a tree edge / stiff graph element (no capacity).
- Serenity: ServoDriveLaw (one mapping ConfigurableJointData -> 1-DOF drive, shared by HeadlessServoPlant and the Jolt backend),
  ServoPhysicsOptions(opts) installs the hooks.

## Steps

1. [ ] PhysicsWorld articulated constraints + world-level drive-law tests (Python reference)
2. [ ] VesselPhysics servo bodies (split, mass, drag, loads, hooks)
3. [ ] Serenity backend + FI partHasServoRb
4. [ ] Scenario gates (hinge, piston, rotor, locked), determinism 1 / 4 threads
5. [ ] Docs (STATUS WP-52b, README), merge main, fresh full verification
