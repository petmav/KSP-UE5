#!/usr/bin/env python3
"""WP-24: independent reference for the stability assist controller and the reaction wheel torque / resource maths -- test data generator.

Re-implements, in plain Python (numpy float32 stores) and from the decompiled C#, the numeric core of
  VesselAutopilot.VesselSAS.ControlUpdate and what it calls (VesselAutopilot.cs:838: rotation delta, CheckCoasting, StabilityDecay,
  AutoTuneScalar/EvaluateScalar, the three PIDclamp.Update (PIDclamp.cs), CheckDamping, the clamp to the control state),
  VesselSAS.GetTotalVesselTorque / UpdateMaximumAcceleration (VesselAutopilot.cs:532, :566),
  ModuleReactionWheel.ActiveUpdate / GetPotentialTorque (ModuleReactionWheel.cs:158, :287) with the resource draw of
  ModuleResourceHandler.UpdateModuleResourceInputs (ModuleResourceHandler.cs:142).
It shares NO code with kspcore. Float32 stores follow the Mono float rule: every float expression is evaluated in double and rounded only where
the C# stores / passes / returns it as a float (st() below); numpy.float32 scalars are used for the per-operation float32 arithmetic of
Unity's native code (Quaternion.eulerAngles).

Inputs (original game data, never hand-authored):
  testdata/GameData-mirror/GameData/Squad/Parts/**/*.cfg   the stock parts with a ModuleReactionWheel: PitchTorque / YawTorque / RollTorque,
                                                            torqueResponseSpeed, the ElectricCharge RESOURCE rate and the part ElectricCharge storage
The dynamic inputs of a run (attitude trajectory, angular velocity, pilot input, ctrl state) are a recorded-style trace drawn from a seeded numpy
generator and stored in the output, so the C++ test replays exactly the values the reference saw.

Unity's native Quaternion.eulerAngles is the only formulation that is not from the decompiled C# (the managed Internal_MakePositive is): the
ToEulerRad matrix form used here is the one of Unity's Math/Quaternion.cpp (YXZ order), written independently of the C++ port; both are
unverified against the native player, see STATUS.md.

Usage: python -I core/tools/wp24/sas_reference.py > core/tests/data/wp24_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
F = np.float32


def st(x):
    """Mono float store: round a double expression to float32 (kept as a Python float)."""
    return float(F(x))


C = lambda v: st(v)  # a float literal of the C# source

# ------------------------------------------------------------------------------------------------ stock part data (original cfgs)


def strip_comments(text):
    return "\n".join(l.split("//")[0] for l in text.splitlines())


def parse_blocks(text):
    """Tiny ConfigNode reader: returns the list of top-level (name, values, children)."""
    tokens = []
    for line in strip_comments(text).splitlines():
        line = line.strip()
        if not line:
            continue
        tokens.append(line)
    pos = 0

    def node(name):
        nonlocal pos
        values, children = [], []
        # expect '{'
        while pos < len(tokens) and tokens[pos] != "{":
            pos += 1
        pos += 1
        while pos < len(tokens):
            t = tokens[pos]
            if t == "}":
                pos += 1
                break
            if "=" in t:
                k, v = t.split("=", 1)
                values.append((k.strip(), v.strip()))
                pos += 1
            elif t == "{":
                pos += 1
            else:
                pos += 1
                children.append((t, *node(t)))
        return values, children

    out = []
    while pos < len(tokens):
        t = tokens[pos]
        pos += 1
        if t == "{" or t == "}" or "=" in t:
            continue
        out.append((t, *node(t)))
    return out


def val(values, key, default=None):
    for k, v in values:
        if k == key:
            return v
    return default


def stock_reaction_wheels():
    root = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Parts")
    parts = []
    for d, _, files in os.walk(root):
        for f in sorted(files):
            if not f.endswith(".cfg"):
                continue
            with open(os.path.join(d, f), encoding="utf-8-sig", errors="replace") as fh:
                text = fh.read()
            for name, values, children in parse_blocks(text):
                if name != "PART":
                    continue
                pname = val(values, "name")
                ec = None
                for cn, cv, cc in children:
                    if cn == "RESOURCE" and val(cv, "name") == "ElectricCharge":
                        ec = (float(val(cv, "amount", "0")), float(val(cv, "maxAmount", "0")))
                for cn, cv, cc in children:
                    if cn != "MODULE" or val(cv, "name") != "ModuleReactionWheel":
                        continue
                    rate = None
                    for rn, rv, rc in cc:
                        if rn in ("RESOURCE", "INPUT_RESOURCE") and val(rv, "name") == "ElectricCharge":
                            rate = float(val(rv, "rate"))
                    parts.append({
                        "part": pname,
                        "PitchTorque": st(float(val(cv, "PitchTorque", "5"))),
                        "YawTorque": st(float(val(cv, "YawTorque", "5"))),
                        "RollTorque": st(float(val(cv, "RollTorque", "5"))),
                        "torqueResponseSpeed": st(float(val(cv, "torqueResponseSpeed", "30"))),
                        "rate": rate,
                        "ec_amount": ec[0] if ec else None,
                        "ec_max": ec[1] if ec else None,
                    })
    parts.sort(key=lambda p: p["part"])
    return parts


# ------------------------------------------------------------------------------------------------ vector / quaternion helpers (Unity managed code)


def v_mul_s(v, s):
    return tuple(st(c * s) for c in v)


def v_add(a, b):
    return tuple(st(x + y) for x, y in zip(a, b))


def v_sub(a, b):
    return tuple(st(x - y) for x, y in zip(a, b))


def v_neg(a):
    return tuple(-c for c in a)


def v_lerp(a, b, t):
    t = 0.0 if t < 0.0 else (1.0 if t > 1.0 else t)
    return tuple(st(x + (y - x) * t) for x, y in zip(a, b))


def v_normalize(v):
    num = st(math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))
    if num > C(1e-05):
        return tuple(st(c / num) for c in v)
    return (0.0, 0.0, 0.0)


def q_mul(l, r):
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    return (st(lw * rx + lx * rw + ly * rz - lz * ry), st(lw * ry + ly * rw + lz * rx - lx * rz),
            st(lw * rz + lz * rw + lx * ry - ly * rx), st(lw * rw - lx * rx - ly * ry - lz * rz))


def q_inv(q):  # Quaternion.Inverse as measured (CAPTURE-SPEC C3): the plain conjugate
    return (-q[0], -q[1], -q[2], q[3])


def q_rot(q, p):  # Quaternion.cs:181
    x, y, z, w = q
    n1 = st(x * 2.0)
    n2 = st(y * 2.0)
    n3 = st(z * 2.0)
    n4 = st(x * n1)
    n5 = st(y * n2)
    n6 = st(z * n3)
    n7 = st(x * n2)
    n8 = st(x * n3)
    n9 = st(y * n3)
    n10 = st(w * n1)
    n11 = st(w * n2)
    n12 = st(w * n3)
    return (st((1.0 - (n5 + n6)) * p[0] + (n7 - n12) * p[1] + (n8 + n11) * p[2]),
            st((n7 + n12) * p[0] + (1.0 - (n4 + n6)) * p[1] + (n9 - n10) * p[2]),
            st((n8 - n11) * p[0] + (n9 + n10) * p[1] + (1.0 - (n4 + n5)) * p[2]))


def euler_angles(q):
    """Quaternion.eulerAngles = Internal_MakePositive(Internal_ToEulerRad(q) * 57.29578f); the native part in per-operation float32."""
    x, y, z, w = (F(c) for c in q)
    two = F(2.0)
    x2, y2, z2 = x * two, y * two, z * two
    xx, yy, zz = x * x2, y * y2, z * z2
    xy, xz, yz = x * y2, x * z2, y * z2
    wx, wy, wz = w * x2, w * y2, w * z2
    one = F(1.0)
    m = [one - (yy + zz), xy + wz, xz - wy, xy - wz, one - (xx + zz), yz + wx, xz + wy, yz - wx, one - (xx + yy)]
    g = lambda r, c: m[r + 3 * c]
    asin = lambda a: F(math.asin(float(a)))
    atan2 = lambda a, b: F(math.atan2(float(a), float(b)))
    pi = F(3.14159274)
    half = F(0.5)
    if g(1, 2) < F(0.999):
        if g(1, 2) > F(-0.999):
            v = (asin(-g(1, 2)), atan2(g(0, 2), g(2, 2)), atan2(g(1, 0), g(1, 1)))
        else:
            v = (pi * half, atan2(g(0, 1), g(0, 0)), F(0.0))
    else:
        v = (-pi * half, atan2(-g(0, 1), g(0, 0)), F(0.0))
    e = [st(float(c) * C(57.29578)) for c in v]
    num = C(-0.005729578)
    num2 = st(360.0 + num)
    for i in range(3):
        if e[i] < num:
            e[i] = st(e[i] + 360.0)
        elif e[i] > num2:
            e[i] = st(e[i] - 360.0)
    return tuple(e)


def fdiv(a, b):  # double division with IEEE results for a zero divisor
    if b == 0.0:
        if a == 0.0 or a != a:
            return float("nan")
        return math.copysign(float("inf"), a) * math.copysign(1.0, b)
    return a / b


def sign(v):
    return -1 if v < 0 else (1 if v > 0 else 0)


def clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)


# ------------------------------------------------------------------------------------------------ PIDclamp.cs


class PID:
    def __init__(self, kp, ki, kd, clamp_):
        self.kp, self.ki, self.kd, self.clamp = float(st(kp)), float(st(ki)), float(st(kd)), float(st(clamp_))
        self.tuning = 1.0
        self.kpS = self.kiS = self.kdS = self.clampS = 1.0
        self.integral = self.integral_last = self.derivative = self.error_last = self.output = 0.0

    def reset(self):
        self.integral = 0.0
        self.error_last = 0.0

    def update(self, error, dt):
        self.integral_last = self.integral
        self.integral += error * dt
        self.derivative = (error - self.error_last) / dt
        self.error_last = error
        self.output = (self.kp * self.tuning * self.kpS * error + self.ki * self.tuning * self.kiS * self.integral
                       + self.kd * self.tuning * self.kdS * self.derivative)
        num = self.clamp * self.clampS
        if self.output < -num:
            self.output = -num
            self.integral = self.integral_last
        elif self.output > num:
            self.output = num
            self.integral = self.integral_last
        return self.output


# ------------------------------------------------------------------------------------------------ VesselSAS


class SAS:
    def __init__(self, ref_rotation, moi, torque_vec, locked, auto_tune=True, use_decay=True, warp_rate=None, target=None):
        self.p = PID(15000.0, 0.0, 2000.0, 1500.0)
        self.r = PID(9000.0, 0.0, 600.0, 2000.0)
        self.y = PID(15000.0, 0.0, 2000.0, 1500.0)
        self.locked_mode = locked
        self.damping_mode = False
        self.override_min = C(0.1)
        self.sas_tuning = 1.0
        self.auto_tune = auto_tune
        self.use_decay = use_decay
        self.warp_rate = warp_rate  # None: no TimeWarp object (rate 1)
        self.moi = moi
        self.torque_provider_sum = torque_vec  # what GetTotalVesselTorque returns for the vessel
        self.torque_vector = (0.0, 0.0, 0.0)
        self.aam = (0.0, 0.0, 0.0)
        self.auto_scalar = (1.0, 1.0, 1.0)
        self.decay = [1.0, 1.0, 1.0]
        self.decay_min = C(0.1)
        self.decay_rate = C(0.005)
        self.decay_resp = C(0.02)
        self.decay_delta = C(0.1)
        self.decay_locked = False
        self.cooldown_timer = 20
        self.cooldown = 20
        self.stop_scalar = 1.0
        self.coast_scalar = 2.0
        self.sas_response = [0.0, 0.0, 0.0]
        self.ang_delta_rad = [0.0, 0.0, 0.0]
        self.rotation_delta = [0.0, 0.0, 0.0]
        self.current = (0.0, 0.0, 0.0, 1.0)
        self.last = (0.0, 0.0, 0.0, 1.0)
        self.locked_rotation = (0.0, 0.0, 0.0, 0.0)
        self.target = (0.0, 0.0, 0.0)
        self.kills = 0
        # ConnectFlyByWire(reset: true)
        self.locked_rotation = ref_rotation
        for pid in (self.p, self.r, self.y):
            pid.reset()
        if target is not None:  # SetTargetOrientation: normalized, CheckStoredTransform changes nothing
            self.target = v_normalize(target)

    def reset_all(self):
        for pid in (self.p, self.r, self.y):
            pid.reset()

    def rate(self):
        return 1.0 if self.warp_rate is None else self.warp_rate

    def tw_delta_time(self, dt):  # TimeWarp.deltaTime
        return dt if self.warp_rate is None else st(dt * self.warp_rate)

    def evaluate_scalar(self, ratio):
        if ratio <= 0.0:
            return 10.0
        if ratio <= 0.1:
            return st(C(-90.0) * ratio + C(10.0))
        if ratio <= 10.0:
            return 1.0
        if ratio <= 50.0:
            return st(C(-0.0167) * ratio + C(1.1667))
        return 0.5

    def auto_tune_scalar(self):
        rate = self.rate()
        if self.auto_tune:
            mn, mx = C(0.5), C(10.0)
            sc = []
            for i in range(3):
                e = self.evaluate_scalar(self.aam[i])
                value = mx if mx < e else e  # Mathf.Min(autoScalarMax, EvaluateScalar(...))
                sc.append(clamp(value, mn, mx))
            self.auto_scalar = tuple(sc)
            for pid, i in ((self.p, 0), (self.r, 1), (self.y, 2)):
                pid.tuning = st(self.sas_tuning * self.auto_scalar[i] / rate)
                pid.clampS = st(rate / self.decay[i])
        else:
            t = st(self.sas_tuning / rate)
            for pid in (self.p, self.r, self.y):
                pid.tuning = t

    def check_coasting(self, w, dt):
        for ax, pid in ((0, self.p), (2, self.y)):
            if self.aam[ax] > 0.0:
                num = abs(st(dt + fdiv(w[ax], self.aam[ax])))
                num2 = abs(st(fdiv(self.rotation_delta[ax] * C(math.pi / 180.0), w[ax])))
                if num * self.stop_scalar > num2 and sign(self.rotation_delta[ax]) != sign(w[ax]):
                    pid.reset()
                    self.rotation_delta[ax] = st(0.0 - self.rotation_delta[ax])
                elif num * self.coast_scalar > num2 and sign(self.rotation_delta[ax]) != sign(w[ax]):
                    pid.reset()
                    self.rotation_delta[ax] = 0.0

    def stability_decay(self):
        if self.use_decay:
            for i in range(3):
                if abs(self.sas_response[i]) < self.decay_resp and self.ang_delta_rad[i] < self.decay_delta:
                    self.decay[i] = max_f(st(self.decay[i] - self.decay_rate), self.decay_min)
                else:
                    self.decay[i] = min_f(st(self.decay[i] + C(0.1)), 1.0)
            if self.vec_eq(self.decay, [self.decay_min] * 3):
                if not self.decay_locked:
                    self.decay_locked = True
                    self.locked_rotation = self.current
                    self.kills += 1
                    self.reset_all()
            else:
                self.decay_locked = False
        else:
            self.decay = [1.0, 1.0, 1.0]

    @staticmethod
    def vec_eq(a, b):  # Vector3.operator== (Vector3.cs:445): float locals
        dx, dy, dz = st(a[0] - b[0]), st(a[1] - b[1]), st(a[2] - b[2])
        return st(dx * dx + dy * dy + dz * dz) < C(9.9999994e-11)

    def check_damping(self, w, ctrl, active, in_p, in_r, in_y):
        self.damping_mode = False
        inputs = []
        for i, (flag, name) in enumerate(((in_p, 0), (in_r, 1), (in_y, 2))):
            c = ctrl[i]
            r = self.sas_response
            upd = False
            if active and flag and c != 0.0:
                if c < 0.0:
                    if not (r[i] < 0.0):
                        r[i] = c
                        upd = True
                    elif c < r[i]:
                        r[i] = c
                        upd = True
                else:
                    if not (r[i] > 0.0):
                        r[i] = c
                        upd = True
                    elif c > r[i]:
                        r[i] = c
                        upd = True
            inputs.append(upd)
        pids = (self.p, self.r, self.y)
        for i in range(3):
            if inputs[i]:
                self.locked_rotation = self.current
                pids[i].reset()
                self.damping_mode = True
        if not any(inputs):
            self.cooldown_timer -= 1
            if self.cooldown_timer < 0:
                self.cooldown_timer = 0
        else:
            self.cooldown_timer = self.cooldown
        if abs(w[0]) > self.override_min:
            self.damping_mode = True
            self.locked_rotation = self.last
            if self.locked_mode:
                if self.cooldown_timer > 0:
                    self.p.reset()
                else:
                    self.reset_all()
        if abs(w[1]) > self.override_min:
            self.damping_mode = True
            self.locked_rotation = self.last
            if self.cooldown_timer > 0:
                self.r.reset()
            else:
                self.reset_all()
        if not (abs(w[2]) > self.override_min):
            return
        self.damping_mode = True
        self.locked_rotation = self.last
        if self.locked_mode:
            if self.cooldown_timer > 0:
                self.y.reset()
            else:
                self.reset_all()

    def control_update(self, ref_rotation, w, dt_frame, ctrl, active, in_p, in_r, in_y):
        """One OnAutopilotUpdate: ControlUpdate, then UpdateVesselTorque. Returns the ctrl state the pilot-facing outputs end up in."""
        self.current = ref_rotation
        self.aam = tuple(max_f(st(self.torque_vector[i] / self.moi[i]), C(0.0001)) for i in range(3))
        delta_q = q_mul(q_inv(self.current), self.locked_rotation)
        self.rotation_delta = list(euler_angles(q_inv(delta_q)))
        if not self.locked_mode:
            # PitchYawAngle(vessel.ReferenceTransform, targetOrientation, out neededPitch, out neededYaw)
            vec = q_rot(q_inv(ref_rotation), self.target)
            pitch = st(90.0 - st(math.atan2(vec[1], vec[2])) * C(57.29578))
            if pitch > 180.0:
                pitch = st(pitch - 360.0)
            yaw = st(st(math.atan2(vec[0], vec[1])) * C(57.29578))
            self.rotation_delta[0] = st(0.0 - pitch)
            self.rotation_delta[2] = yaw
            self.check_coasting(w, dt_frame)
        elif not self.damping_mode:
            self.check_coasting(w, dt_frame)
        ang = [self.rotation_delta[i] if not (self.rotation_delta[i] > 180.0) else st(self.rotation_delta[i] - 360.0) for i in range(3)]
        self.ang_delta_rad = [a * 0.01745329238474369 for a in ang]
        self.stability_decay()
        self.auto_tune_scalar()
        dt = float(self.tw_delta_time(dt_frame))
        self.sas_response[0] = self.p.update(self.ang_delta_rad[0], dt) / self.p.clamp
        self.sas_response[1] = self.r.update(self.ang_delta_rad[1], dt) / self.r.clamp
        self.sas_response[2] = self.y.update(self.ang_delta_rad[2], dt) / self.y.clamp
        self.check_damping(w, ctrl, active, in_p, in_r, in_y)
        for i in range(3):
            self.sas_response[i] = clamp(self.sas_response[i], -1.0, 1.0)
        out = tuple(st(self.sas_response[i]) for i in range(3))
        self.last = self.current
        # UpdateVesselTorque (runs after ControlUpdate: the delegate order)
        self.torque_vector = tuple(max(self.torque_provider_sum[0][i], self.torque_provider_sum[1][i]) for i in range(3))
        return out


def max_f(a, b):  # Mathf.Max
    return a if a > b else b


def min_f(a, b):  # Mathf.Min
    return a if a < b else b


# ------------------------------------------------------------------------------------------------ trace generation


def rand_rot(rng, ang_deg):
    axis = rng.normal(size=3)
    axis /= np.linalg.norm(axis)
    a = math.radians(ang_deg) / 2.0
    s = math.sin(a)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(a))


def q_norm64(q):
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def q_mul64(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by + ay * bw + az * bx - ax * bz,
            aw * bz + az * bw + ax * by - ay * bx, aw * bw - ax * bx - ay * by - az * bz)


def make_scenario(name, seed, steps, locked, parts, moi, warp_rate=None, auto_tune=True, use_decay=True, target=None,
                  drift_deg=0.05, spin=0.01, bursts=(), pilot=()):
    rng = np.random.default_rng(seed)
    q0 = tuple(st(c) for c in q_norm64(rand_rot(rng, 40.0)))
    cur = q0
    quats, angvels, ctrls, flags = [], [], [], []
    for i in range(steps):
        step = rand_rot(rng, abs(rng.normal()) * drift_deg)
        cur64 = q_norm64(q_mul64(cur, step))
        cur = tuple(st(c) for c in cur64)
        quats.append(cur)
        w = [st(c) for c in rng.normal(size=3) * spin]
        for (a, b, ax, mag) in bursts:
            if a <= i < b:
                w[ax] = st(mag)
        angvels.append(tuple(w))
        c = [0.0, 0.0, 0.0]
        f = [False, False, False]
        for (a, b, ax, mag) in pilot:
            if a <= i < b:
                c[ax] = st(mag)
                f[ax] = True
        ctrls.append(tuple(c))
        flags.append(tuple(f))
    # torque providers: parts from last to first (GetTotalVesselTorque order), float adds
    pos = (0.0, 0.0, 0.0)
    neg = (0.0, 0.0, 0.0)
    for p in reversed(parts):
        if p["state"] == "Active" and p["actuatorModeCycle"] != 2:
            t = (p["PitchTorque"], p["RollTorque"], p["YawTorque"])
            pos = v_add(pos, t)
            neg = v_add(neg, t)
    sas = SAS(q0, moi, (pos, neg), locked, auto_tune, use_decay, warp_rate, target)
    dt = st(0.02)
    out = []
    for i in range(steps):
        res = sas.control_update(quats[i], angvels[i], dt, ctrls[i], True, flags[i][0], flags[i][1], flags[i][2])
        out.append({
            "ctrl": list(res),
            "sasResponse": list(sas.sas_response),
            "rotationDelta": list(sas.rotation_delta),
            "angularDeltaRad": list(sas.ang_delta_rad),
            "angularAccelerationMax": list(sas.aam),
            "decayScalar": list(sas.decay),
            "lockedRotation": list(sas.locked_rotation),
            "dampingMode": sas.damping_mode,
            "tuningScalar": [sas.p.tuning, sas.r.tuning, sas.y.tuning],
            "clampScalar": [sas.p.clampS, sas.r.clampS, sas.y.clampS],
            "torqueVector": list(sas.torque_vector),
            "pidOutput": [sas.p.output, sas.r.output, sas.y.output],
        })
    return {
        "name": name, "locked": locked, "autoTune": auto_tune, "useDecay": use_decay, "warpRate": warp_rate, "moi": list(moi),
        "target": list(target) if target else None, "parts": parts, "q0": list(q0), "deltaTime": dt,
        "steps": [{"q": list(quats[i]), "w": list(angvels[i]), "pilot": list(ctrls[i]), "input": list(flags[i])} for i in range(steps)],
        "expected": out, "killAngularVelocityCalls": sas.kills,
    }


# ------------------------------------------------------------------------------------------------ ModuleReactionWheel


def request_electric_charge(amount, demand, threshold=1e-12):
    """Part.RequestResource of ElectricCharge (STAGE_PRIORITY_FLOW) on a vessel whose only ElectricCharge store is this part: PartSet.ProcessRequest
    (PartSet.cs:265, usePri branch, one priority list holding one resource) with Part.TransferResource (Part.cs:6605) on the pulling side.
    Returns (amount handed out, amount left). The proportional drain num5 * amount is not exactly -demand: the residue is why the stored
    amount is compared with this model instead of `amount - demand`."""
    def pull(res, x):  # TransferResource with a negative amount
        a = 0.0 - x
        if a >= res:
            return res, 0.0
        return a, res - a

    num = demand
    num2 = amount
    if num2 == 0.0:
        return demand - num, amount
    flag = num2 > num
    num4 = 0.0
    left = amount
    if flag:
        num5 = (0.0 - num) / num2
        num6 = num5 * amount
        got, left = pull(left, num6)
        num4 += got
        num -= num4
    flag2 = True
    if (not flag) or num == demand:
        num4 = 0.0
        flag2 = False
        got, left = pull(left, 0.0 - demand)
        num4 += got
        num7 = num - num4
        if num7 <= threshold:
            return demand - num7, left
        num -= num4
    return demand - num, left


def wheel_case(spec, seed, steps=48):
    """ModuleReactionWheel.ActiveUpdate over a trace; resource draw from ModuleResourceHandler (one part, ElectricCharge)."""
    rng = np.random.default_rng(seed)
    P, Y, R = spec["PitchTorque"], spec["YawTorque"], spec["RollTorque"]
    trs = spec["torqueResponseSpeed"]
    fdt = st(0.02)
    iv = (0.0, 0.0, 0.0)
    ec = spec["ec_start"]
    out = []
    ins = []
    for i in range(steps):
        mode = i % 3
        authority = st(rng.uniform(0.0, 100.0)) if i % 7 else 100.0
        ctrl = tuple(st(c) for c in rng.uniform(-1.0, 1.0, size=3))   # pitch, roll, yaw of vessel.ctrlState
        cmd = tuple(st(c) for c in rng.uniform(-1.0, 1.0, size=3))    # FlightInputHandler.state.GetPYR()
        active = bool((i // 3) % 2 == 0)
        ap_enabled = bool((i // 6) % 2 == 0)
        q = tuple(st(c) for c in q_norm64(rand_rot(rng, 90.0)))
        rec = {"mode": mode, "authority": authority, "ctrl": list(ctrl), "cmd": list(cmd), "activeVessel": active,
               "autopilotEnabled": ap_enabled, "q": list(q)}
        ins.append(rec)
        res = {"updated": False}
        if ap_enabled or mode != 1:
            ivt = ctrl
            vcmd = cmd if active else (0.0, 0.0, 0.0)
            if mode == 2:
                ivt = vcmd
            elif mode == 1:
                ivt = v_sub(ivt, vcmd)
            ivt = (st(ivt[0] * P), st(ivt[1] * R), st(ivt[2] * Y))
            ivt = v_mul_s(ivt, st(authority * C(0.01)))
            iv = v_lerp(iv, ivt, st(trs * fdt))
            input_sum = st((abs(ctrl[0]) + abs(ctrl[1]) + abs(ctrl[2])) * (authority * C(0.01)))
            operational = False
            request = 0.0
            taken = 0.0
            if input_sum > 0.0:
                if spec["rate"] is None:
                    operational = True   # no input resources: UpdateModuleResourceInputs returns 1.0
                else:
                    request = (spec["rate"] * input_sum) * float(fdt)   # rate * rateMultiplier, then * fixedDeltaTime (ModuleResourceHandler.cs:160-162)
                    taken, ec = request_electric_charge(ec, request)
                    threshold = 0.9
                    if taken < request * max(threshold, 0.001):
                        num = taken / request
                        operational = num > 0.0
                    else:
                        operational = True
            torque = q_rot(q, v_neg(iv)) if operational else None
            res = {"updated": True, "inputVectorTemp": list(ivt), "inputVector": list(iv), "inputSum": input_sum,
                   "operational": operational, "torque": list(torque) if torque else None, "request": request, "ec": ec}
        out.append(res)
    return {"spec": spec, "steps": ins, "expected": out, "deltaTime": fdt}


def main():
    wheels = stock_reaction_wheels()
    assert len(wheels) >= 6, "expected stock reaction wheels in testdata"
    by_name = {w["part"]: w for w in wheels}

    def sas_part(name, state="Active", cycle=0):
        w = dict(by_name[name])
        w["state"] = state
        w["actuatorModeCycle"] = cycle
        return w

    names = sorted(by_name)
    pa, pb, pc = names[0], names[len(names) // 2], names[-1]
    scenarios = [
        make_scenario("locked_stability_assist", 2401, 300, True, [sas_part(pa), sas_part(pb)], (st(3.1), st(2.2), st(3.0)),
                      bursts=((60, 70, 0, 0.15), (150, 156, 1, -0.2)), pilot=((120, 140, 0, 0.4), (200, 210, 2, -0.2))),
        make_scenario("target_orientation", 2402, 300, False, [sas_part(pa), sas_part(pc)], (st(1.4), st(0.9), st(1.5)),
                      target=(st(0.3), st(0.8), st(-0.5)), drift_deg=0.4, spin=0.03),
        make_scenario("warp_rate_5_no_autotune", 2403, 300, True, [sas_part(pa), sas_part(pb, cycle=2), sas_part(pc)],
                      (st(6.5), st(5.0), st(6.0)), warp_rate=5.0, auto_tune=False, drift_deg=0.002, spin=0.001),
        make_scenario("no_decay_autotune", 2404, 200, True, [sas_part(pb, state="Disabled"), sas_part(pc)], (st(0.4), st(0.3), st(0.45)),
                      use_decay=False, drift_deg=0.3, spin=0.02, bursts=((40, 50, 2, 0.5),)),
    ]
    cases = []
    for k, w in enumerate(wheels):
        spec = dict(w)
        # the part ElectricCharge storage is the supply; a quarter of the wheels start nearly empty to exercise the shortfall branch
        cap = w["ec_max"] if w["ec_max"] is not None else 0.0
        spec["ec_start"] = cap if k % 4 else cap * 0.003
        cases.append(wheel_case(spec, 7000 + k))
    json.dump({"wp": 24, "generator": "tools/wp24/sas_reference.py", "sas": scenarios, "wheels": cases}, sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
