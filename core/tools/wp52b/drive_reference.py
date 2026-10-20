#!/usr/bin/env python3
"""WP-52b independent reference: the joint drive law of the robotic servos, replayed by tests/test_servo_backend.cpp.

No kspcore code. What KSP hands PhysX is the ConfigurableJoint's drive (Expansions/Serenity/ModuleRoboticServoHinge.cs:467-469,
ModuleRoboticServoPiston.cs:429-431, ModuleRoboticServoRotor.cs:637-648, ModuleRoboticRotationServo.cs:478-480): positionSpring k,
positionDamper c, maximumForce F, and the targets (targetRotation / targetPosition / targetAngularVelocity). PhysX's documented D6 drive
(PxD6JointDrive, force mode) is

    F = k (x_target - x) + c (v_target - v),  |F| <= maximumForce,

integrated implicitly by the solver (the drive is a soft constraint of the velocity solve). For one degree of freedom of effective inertia
I (kg m^2 or tonnes m^2; the reduced model's units are KSP's, so torque is kN m) and step dt:

    v1 = (v0 + dt/I (k (xt - x0) + c vt)) / (1 + dt/I (c + dt k)),   F = I (v1 - v0) / dt, clamped to +-maximumForce (then v1 = v0 + dt F / I),
    x1 = x0 + dt v1.

This is the law core/src/serenity/servo_joint.cpp StepDrive1D documents (written there from the same reasoning: an independent
re-derivation here, no shared code). UNVERIFIED against PhysX itself (no capture): the sign conventions and the limit / spring behaviour.

Cases (fixed seed, every value a SYNTHETIC fixture: no servo part data exists locally):
  position  : a position drive from rest to a target (hinge / piston), several k, c, F, I;
  ramp      : a MoveTowards ramp of the target at a fixed rate (the servos' traverseVelocity), the lag tends to c / k * rate;
  velocity  : a velocity drive (the rotor: k = 1e-10, damper c, targetVelocity vt) under a torque limit F: dv/dt = F / I until vt;
  hold      : a position drive holding a constant external load L (the locked servo): rest offset L / k.
The analytic markers (lag, spool time, rest offset) are emitted next to the discrete trajectories.

Output: core/tests/data/wp52b_reference.json.  Usage:  python -I core/tools/wp52b/drive_reference.py
"""
import json
import math
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp52b_reference.json")

DT = 0.02  # Time.fixedDeltaTime of the flight scene


def step(x, v, I, k, c, fmax, xt, vt, load, dt):
    """One implicit step of the drive plus a constant external force / torque `load` (explicit, as a body force)."""
    a = dt / I
    v1 = (v + a * (k * (xt - x) + c * vt) + a * load) / (1.0 + a * (c + dt * k))
    f = I * (v1 - v) / dt - load  # the drive's share
    if abs(f) > fmax:
        f = math.copysign(fmax, f)
        v1 = v + dt * (f + load) / I
    return x + dt * v1, v1, f


def run(case):
    x, v = case["x0"], case["v0"]
    xs, vs, fs = [], [], []
    for n in range(case["steps"]):
        xt = case["target"]
        if case["kind"] == "ramp":  # Vector3.MoveTowards / Mathf.MoveTowards of the drive target by rate * dt per tick
            xt = min(case["x0"] + case["rate"] * DT * (n + 1), case["target"])
        x, v, f = step(x, v, case["I"], case["k"], case["c"], case["F"], xt, case["vt"], case["load"], DT)
        xs.append(x)
        vs.append(v)
        fs.append(f)
    out = dict(case)
    out["x"], out["v"], out["f"] = xs, vs, fs
    # analytic markers
    if case["kind"] == "ramp":
        out["lag"] = case["c"] / case["k"] * case["rate"]  # steady lag of an overdamped drive behind a ramp (continuous limit)
    if case["kind"] == "velocity":
        out["spool"] = case["I"] * case["vt"] / case["F"]  # time to reach vt at the torque limit (c large enough to saturate)
    if case["kind"] == "hold":
        out["offset"] = case["load"] / case["k"]  # rest offset under a constant load
    return out


def main():
    rng = random.Random(5252)
    cases = []
    for i in range(12):  # position drives (hinge: rad; piston: m), drive forces from the servo formulas' range (SYNTHETIC)
        I = rng.choice([0.05, 0.2, 1.0, 4.0])
        k = rng.choice([10.0, 50.0, 200.0, 1000.0])
        c = k * rng.choice([0.05, 0.2, 0.5])
        F = rng.choice([1.0e30, 5.0, 20.0])
        cases.append(dict(name="position %d" % i, kind="position", I=I, k=k, c=c, F=F, x0=0.0, v0=0.0,
                          target=rng.uniform(-1.4, 1.4), vt=0.0, load=0.0, steps=200))
    for i in range(6):
        k = rng.choice([100.0, 500.0])
        cases.append(dict(name="ramp %d" % i, kind="ramp", I=rng.choice([0.1, 1.0]), k=k, c=0.2 * k, F=1.0e30, x0=0.0, v0=0.0,
                          target=1.2, vt=0.0, load=0.0, rate=rng.choice([0.3, 0.6]), steps=300))
    for i in range(6):  # the rotor: spring 1e-10 (ModuleRoboticServoRotor angularPositionSpring), damper large, torque-limited
        I = rng.choice([0.5, 2.0])
        F = rng.choice([2.0, 10.0])
        cases.append(dict(name="velocity %d" % i, kind="velocity", I=I, k=1.0e-10, c=rng.choice([1.0e3, 1.0e4]), F=F, x0=0.0, v0=0.0,
                          target=0.0, vt=rng.choice([5.0, 20.0, 40.0]), load=0.0, steps=400))
    for i in range(4):  # locked servo: the drive holds against a constant load
        k = rng.choice([200.0, 1000.0])
        cases.append(dict(name="hold %d" % i, kind="hold", I=rng.choice([0.2, 1.0]), k=k, c=0.3 * k, F=1.0e30, x0=0.0, v0=0.0,
                          target=0.0, vt=0.0, load=rng.choice([-20.0, 15.0]), steps=400))
    data = {"dt": DT, "cases": [run(c) for c in cases]}
    with open(OUT, "w", newline="\n") as f:
        json.dump(data, f, indent=0, sort_keys=True)
        f.write("\n")


if __name__ == "__main__":
    main()
