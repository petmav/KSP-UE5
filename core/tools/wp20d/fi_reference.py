#!/usr/bin/env python3
"""WP-20d independent reference: the FlightIntegrator.FixedUpdate statements the tick adds around the WP-09 / 21 / 22 ports.

No kspcore code. Everything is written from the decompiled C# (Assembly-CSharp):
  FlightIntegrator.cs  SmoothVelocity (:495-542: the 10-sample ring with spike rejection), FixedUpdate's spd / nVel (:603-616),
                       IntegratePhysicalObjects (:747-782: the acceleration at the vessel's CoM, the drag of one object)
  FlightGlobals.cs     getGeeForceAtPosition (:1347), getCentrifugalAcc (:1356), getCoriolisAcc (:1366)
  UnityEngine          Vector3 operator- / sqrMagnitude / magnitude / operator/ (managed, Mono rule), Vector3d (double), the implicit
                       Vector3d -> Vector3 conversion (a float store per component)

The Mono float rule (core/README.md): a C# float expression is evaluated in double and rounded to float32 where the C# stores, returns or
passes it. Python floats hold float32-exact values for C# floats; F() is the float32 store.

Fixtures that are NOT KSP data: every input (velocities with spikes, positions, body constants, densities) is generated here from a fixed
seed; only the code under test is the game's.

Output: core/tests/data/wp20d_reference.json. Doubles are written as 16 hex digits of the IEEE-754 bits, float32 as 8.
Usage:  python -I core/tools/wp20d/fi_reference.py
"""
import json
import math
import os
import random
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp20d_reference.json")

R = random.Random(2004)


def F(x):
    """C# float store of a double expression"""
    return float(np.float32(x))


def hx(x):
    return struct.pack(">d", x).hex()


def hf(x):
    return struct.pack(">f", F(x)).hex()


def H3(v):
    return [hx(c) for c in v]


def HF3(v):
    return [hf(c) for c in v]


# ---------------------------------------------------------------------------------------------------- Vector3 (float, Mono rule)
def f_sub(a, b):
    """Vector3.operator- : new Vector3(a.x - b.x, ...)"""
    return (F(a[0] - b[0]), F(a[1] - b[1]), F(a[2] - b[2]))


def f_sqr(a):
    """Vector3.sqrMagnitude: x * x + y * y + z * z (double evaluation, float return)"""
    return F(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def f_mag(a):
    """Vector3.magnitude: (float)Math.Sqrt(x * x + y * y + z * z)"""
    return F(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


def f_div(a, d):
    """Vector3.operator/(Vector3, float)"""
    return (F(a[0] / d), F(a[1] / d), F(a[2] / d))


def f_of(v):
    """Vector3d -> Vector3"""
    return (F(v[0]), F(v[1]), F(v[2]))


# ---------------------------------------------------------------------------------------------------- Vector3d (double)
def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, d):
    return (a[0] * d, a[1] * d, a[2] * d)


def div(a, d):
    return (a[0] / d, a[1] / d, a[2] / d)


def sqr(a):
    return a[0] * a[0] + a[1] * a[1] + a[2] * a[2]


def cross(l, r):
    return (l[1] * r[2] - l[2] * r[1], l[2] * r[0] - l[0] * r[2], l[0] * r[1] - l[1] * r[0])


def neg(a):
    """Vector3d.operator-(a) (Vector3d.cs:291): new Vector3d(0.0 - a.x, ...): +0 for a zero component"""
    return (0.0 - a[0], 0.0 - a[1], 0.0 - a[2])


def normalized(v):
    """Vector3d.normalized: v / magnitude (zero when the magnitude is not > 0)"""
    n = math.sqrt(sqr(v))
    if n > 0.0:
        return div(v, n)
    return (0.0, 0.0, 0.0)


# ---------------------------------------------------------------------------------------------------- SmoothVelocity
class Smoother:
    LEN = 10

    def __init__(self):
        self.ring = [(0.0, 0.0, 0.0)] * self.LEN
        self.index = 0
        self.recip = 1.0 / float(self.LEN)
        self.provisional = False
        self.spiking = False
        self.last_vel = (0.0, 0.0, 0.0)
        self.last_index = 0
        self.max_sqr = 1000000.0

    def step(self, vel, fdt):
        """FlightIntegrator.cs:495"""
        unsmoothed = vel
        num = self.index - 1
        if num < 0:
            num = self.LEN - 1
        vector = self.ring[num]
        if f_sqr(f_sub(vel, vector)) > self.max_sqr * fdt * fdt and f_sqr(vel) > f_sqr(vector):
            if self.spiking:
                self.ring[self.index] = vel
                self.index += 1
            elif self.provisional:
                self.ring[self.index] = vel
                self.index += 1
                self.ring[self.last_index] = self.last_vel
                self.spiking = True
            else:
                self.provisional = True
                self.last_vel = vel
                self.last_index = self.index
                self.ring[self.index] = vector
                self.index += 1
        else:
            self.ring[self.index] = vel
            self.index += 1
            self.spiking = False
            self.provisional = False
        if self.index >= self.LEN:
            self.index = 0
        zero = (0.0, 0.0, 0.0)
        k = self.LEN
        while k > 0:
            k -= 1
            zero = add(zero, self.ring[k])
        zero = mul(zero, self.recip)
        return unsmoothed, f_of(zero)


def smooth_cases():
    cases = []
    for c in range(40):
        fdt = F([0.02, 0.02, 0.02, 0.04, 0.06, 0.08][c % 6])
        s = Smoother()
        base = (R.uniform(-300, 300), R.uniform(-300, 300), R.uniform(-300, 300))
        steps = []
        for k in range(80):
            # a smooth drift, with spikes of 1-3 frames (KSP's spike rejection) and occasional jumps that stay
            base = add(base, (R.uniform(-2, 2), R.uniform(-2, 2), R.uniform(-2, 2)))
            v = base
            r = R.random()
            if r < 0.08:
                v = add(base, (R.uniform(-3000, 3000), R.uniform(-3000, 3000), R.uniform(-3000, 3000)))
            elif r < 0.10:
                base = add(base, (R.uniform(-2000, 2000), R.uniform(-2000, 2000), R.uniform(-2000, 2000)))
                v = base
            veld = v
            vel = f_of(veld)  # FixedUpdate :603 Vel = Veld
            unsm, out = s.step(vel, fdt)
            spd = f_mag(out)  # :614 (not IgnoreSpeedActive)
            nvel = f_div(out, F(spd)) if spd != 0.0 else (0.0, 0.0, 0.0)  # :616
            steps.append({"veld": H3(veld), "vel": HF3(out), "unsmoothed": HF3(unsm), "index": s.index, "spiking": s.spiking,
                          "provisional": s.provisional, "spd": hx(spd), "nvel": HF3(nvel)})
        cases.append({"fdt": hf(fdt), "steps": steps})
    return cases


# ---------------------------------------------------------------------------------------------------- IntegratePhysicalObjects
def gee(pos, body, g):
    """FlightGlobals.cs:1347"""
    p = sub(pos, body)
    s = sqr(p)
    s *= math.sqrt(s)
    return div(mul(p, 0.0 - g), s)


def centrifugal(pos, body, omega, inv):
    """FlightGlobals.cs:1356"""
    if not inv:
        return (0.0, 0.0, 0.0)
    p = sub(body, pos)
    return cross(omega, cross(omega, p))


def coriolis(vel, omega, inv):
    """FlightGlobals.cs:1366"""
    if not inv:
        return (0.0, 0.0, 0.0)
    c = cross(omega, vel)
    return (-2.0 * c[0], -2.0 * c[1], -2.0 * c[2])


def physical_object_cases():
    cases = []
    for c in range(300):
        body = (R.uniform(-1e7, 1e7), R.uniform(-1e7, 1e7), R.uniform(-1e7, 1e7))
        r = R.uniform(6.0e5, 2.0e6)
        d = normalized((R.uniform(-1, 1), R.uniform(-1, 1), R.uniform(-1, 1)))
        comd = add(body, mul(d, r))
        veld = (R.uniform(-2500, 2500), R.uniform(-2500, 2500), R.uniform(-2500, 2500))
        g = R.uniform(1e12, 4e12)
        omega = (R.uniform(-1e-3, 1e-3), R.uniform(-3e-4, 3e-4), R.uniform(-1e-3, 1e-3))
        inv = R.random() < 0.6
        mult = [1.0, 1.0, 0.5, 2.0][c % 4]
        # :752 (gee + coriolis + centrifugal) * GraviticForceMultiplier -> Vector3
        acc = f_of(mul(add(add(gee(comd, body, g), coriolis(veld, omega, inv)), centrifugal(comd, body, omega, inv)), mult))
        # :767-780 for one object
        rbv = (F(R.uniform(-300, 300)), F(R.uniform(-300, 300)), F(R.uniform(-300, 300)))
        if c % 7 == 0:
            rbv = (0.0, 0.0, 0.0)
        frame = (R.uniform(-2000, 2000), R.uniform(-2000, 2000), R.uniform(-2000, 2000)) if c % 3 else (0.0, 0.0, 0.0)
        if c % 7 == 0:
            frame = (0.0, 0.0, 0.0)
        orig = F(R.uniform(0.1, 3.0))
        dens = [0.0, R.uniform(1e-5, 1.3), R.uniform(1e-5, 1.3), -0.1][c % 4]
        pre = R.uniform(0.5, 1.5)
        dmult = F(8.0)
        apply_drag = c % 11 != 0
        applied = False
        drag = (0.0, 0.0, 0.0)
        if apply_drag and dens > 0.0:
            v = add(rbv, frame)
            num2 = 0.0005 * pre * dens * orig * sqr(v) * dmult
            if not (math.isnan(num2) or math.isinf(num2)):
                n = normalized(v)
                drag = f_of(mul(neg(n), num2))
                applied = True
        cases.append({"body": H3(body), "comd": H3(comd), "veld": H3(veld), "g": hx(g), "omega": H3(omega), "inv": inv, "mult": hx(mult),
                      "acc": HF3(acc), "rbv": HF3(rbv), "frame": H3(frame), "origDrag": hf(orig), "density": hx(dens),
                      "pseudoReDragMult": hx(pre), "dragMultiplier": hf(dmult), "applyDrag": apply_drag, "applied": applied,
                      "drag": HF3(drag)})
    return cases


def main():
    out = {"schema": "wp20d-reference/1", "generator": "core/tools/wp20d/fi_reference.py", "seed": 2004,
           "smooth": smooth_cases(), "physical_objects": physical_object_cases()}
    with open(OUT, "w", newline="\n") as f:
        json.dump(out, f, indent=1, sort_keys=True)
        f.write("\n")
    print(OUT, len(out["smooth"]), "smoothing sequences,", len(out["physical_objects"]), "physical-object cases")


if __name__ == "__main__":
    main()
