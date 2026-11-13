---
name: jolt-servo-solver-findings
description: Measured Jolt solver behaviour for articulated joints on the reduced model (WP-52b, 2026-10-20): GS convergence vs iterations, off-axis rotation dissipation, light-base anchoring
metadata:
  type: project
---

Jolt joint drives on the reduced rigid-body model (WP-52b, measured 2026-10-20):
- Jolt's PositionAndVelocity motor with a StiffnessAndDamping spring IS the implicit PhysX D6 drive law (exact when target velocity 0); it matches an independent implicit reference to float precision when the axis passes the centre of mass.
- With a lever arm (axis 2.4 m off an 18 t arm) Gauss-Seidel coupling to the locked axes leaves ~0.5 % error at 10 velocity steps, 0.06 % at 50, ~0 at 200 -> servo islands get a 50-step per-constraint override (cost within noise).
- A velocity-level solve dissipates (1 - cos(w dt)) of an off-pivot body's tangential speed per step (tens of kN m for a fast heavy rotor arm); not an iteration problem.
- A light body welded to the world between a heavy arm and the anchor (72:1) converges badly at any iteration count.

**Why:** these explain mismatches against analytic 1-DOF references; they are solver properties, not porting bugs.
**How to apply:** when gating wheels / EVA / robotics against 1-DOF references, put axes through centres of mass or raise island iterations before suspecting the port; never loosen an existing tolerance for it. Details in core/STATUS.md "WP-52b".
