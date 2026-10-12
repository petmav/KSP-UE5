#!/usr/bin/env python3
"""WP-19b independent reference: the frame bookkeeping of the physics bubble.

No kspcore code. Everything is written from the decompiled C# (decompiled/KSP-Decompiled/Assembly-CSharp):
  OrbitPhysicsManager.cs  setRotatingFrame (:235-302): every unpacked, living vessel's rigid bodies -/+ body.getRFrmVel(vessel.transform.position),
                          krakensbaneAcc -/+ the same vector
  CelestialBody.cs        getRFrmVel (Vector3d.Cross(angularVelocity, worldPos - position))
  Vessel.cs               ChangeWorldVelocity (:3197): not DEAD / packed: rb.velocity = (Vector3d)rb.velocity + velOffset (float store),
                          krakensbaneAcc += velOffset * (1.0 / (double)TimeWarp.fixedDeltaTime)
  Krakensbane.cs          FixedUpdate (:55), SafeToEngage (:87), AddExcess (:121), Zero (:150)
  FloatingOrigin.cs       FixedUpdate (:125: the continuous / threshold decision), setOffset (:166-206: bodies, vessels' SetPosition and CoMD)
  Vector3d.cs / Vector3   Cross, Dot, sqrMagnitude (Vector3.sqrMagnitude under the Mono rule: evaluated in double, rounded to float once)

kspcore's two deliberate departures (STATUS.md WP-19b, documented in floating_origin.hpp) are modelled as such: the out-of-frame offset of a
Krakensbane tick accumulates in a frame offset instead of shifting the world every tick, and a world shift subtracts the active vessel's
position only (nonFrame = 0). The rigid bodies of a vessel are one cluster here (one-part vessels in the C++ test): the C#'s per-part loop
is the same float operation on each part's rb.velocity.

Fixtures that are NOT KSP data: every input (positions, velocities, flags, rates) is generated here from a fixed seed.

Output: core/tests/data/wp19b_reference.json. Doubles as 16 hex digits of the IEEE-754 bits, float32 as 8; bool / int as JSON.
Usage:  python -I core/tools/wp19b/frames_reference.py
"""
import json
import os
import random
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(CORE, "tests", "data", "wp19b_reference.json")

f32 = np.float32
R = random.Random(1919)


def hx(x):
    return struct.pack(">d", float(x)).hex()


def hf(x):
    return struct.pack(">f", float(f32(x))).hex()


def H3(v):
    return [hx(c) for c in v]


def F3(v):
    return [hf(c) for c in v]


def F(x):
    """C# float store of a double expression"""
    return float(f32(x))


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def neg(a):
    return (-a[0], -a[1], -a[2])


def mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    # Vector3d.Cross
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def is_zero(a):
    return a[0] == 0.0 and a[1] == 0.0 and a[2] == 0.0


def store3(a):
    """rb.velocity = (Vector3d expression): one float rounding per component"""
    return (F(a[0]), F(a[1]), F(a[2]))


def sqr_mag_f(v):
    """Vector3.sqrMagnitude under the Mono rule (float operands, double evaluation, one float store)"""
    return F(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])


def rf():
    return F(R.uniform(-1.0, 1.0))


def rvec(scale):
    return (R.uniform(-scale, scale), R.uniform(-scale, scale), R.uniform(-scale, scale))


def rvecf(scale):
    return store3(rvec(scale))


# ------------------------------------------------------------------------------------------- OrbitPhysicsManager.setRotatingFrame
def rotating_frame_scenes(n):
    out = []
    for _ in range(n):
        body_pos = add((-700000.0, 250000.0, 120000.0), rvec(80000.0))
        w = R.uniform(1e-4, 6e-4)
        ang = add((0.0, -w, 0.0), rvec(2e-5)) if R.random() < 0.5 else (0.0, -w, 0.0)
        rotating = R.random() < 0.5
        vessels = []
        for _ in range(3):
            v = {
                "dead": R.random() < 0.15,
                "packed": R.random() < 0.15,
                "pos": rvecf(4000.0),
                "vel": rvecf(60.0) if R.random() < 0.7 else rvecf(2500.0),
                "kacc": rvec(150.0),
            }
            vessels.append(v)
        res = []
        for v in vessels:
            vel, kacc = v["vel"], v["kacc"]
            if not (v["dead"] or v["packed"]):
                r_frm = cross(ang, sub(v["pos"], body_pos))  # getRFrmVel(vessel.transform.position)
                if rotating:
                    vel = store3(sub(vel, r_frm))
                    kacc = sub(kacc, r_frm)
                else:
                    vel = store3(add(vel, r_frm))
                    kacc = add(kacc, r_frm)
            res.append({"vel": F3(vel), "kacc": H3(kacc)})
        out.append({
            "body_pos": H3(body_pos), "ang": H3(ang), "rotating": rotating,
            "vessels": [{"dead": v["dead"], "packed": v["packed"], "pos": F3(v["pos"]), "vel": F3(v["vel"]), "kacc": H3(v["kacc"])} for v in vessels],
            "out": res,
        })
    return out


# ------------------------------------------------------------------------------------------- Vessel.ChangeWorldVelocity
FDTS = [0.02, 0.04, 0.06, 0.08, 0.1]  # Time.fixedDeltaTime = 0.02f * Time.timeScale during physical warp (TimeWarp.cs:423)


def change_world_velocity_scenes(n):
    out = []
    for _ in range(n):
        dead = R.random() < 0.1
        packed = R.random() < 0.1
        fdt = F(F(0.02) * R.choice([1.0, 2.0, 3.0, 4.0, 5.0])) if R.random() < 0.5 else F(R.choice(FDTS))
        vel = rvecf(80.0) if R.random() < 0.6 else rvecf(3000.0)
        kacc = rvec(500.0)
        dv = rvec(30.0) if R.random() < 0.6 else rvec(2500.0)
        nvel, nkacc = vel, kacc
        if not (dead or packed):
            nvel = store3(add(vel, dv))
            nkacc = add(kacc, mul(dv, 1.0 / fdt))
        out.append({"dead": dead, "packed": packed, "fdt": hf(fdt), "vel": F3(vel), "kacc": H3(kacc), "dv": H3(dv),
                    "out_vel": F3(nvel), "out_kacc": H3(nkacc)})
    return out


# ------------------------------------------------------------------------------------------- FloatingOrigin.FixedUpdate / setOffset
VEL_FOR_CONTINUOUS_SQR = 5000.0 * 5000.0
THRESHOLD_SQR = F(F(500.0) * F(500.0))


def floating_origin_scenes(n, nbodies):
    out = []
    for _ in range(n):
        # the active vessel's transform: mostly within / around the 500 m threshold
        scale = R.choice([300.0, 450.0, 600.0, 3000.0])
        active = rvecf(scale)
        can_engage = R.random() < 0.85
        landed = R.random() < 0.2
        mode_high = R.random() < 0.7
        cur_rate = F(R.choice([1.0, 2.0, 4.0, 5.0, 10.0, 1000.0]))
        max_phys = F(4.0)
        ship_vel = rvecf(R.choice([10.0, 2000.0, 4000.0]))
        out_of_frame = rvec(60.0) if R.random() < 0.5 else (0.0, 0.0, 0.0)
        frame_offset0 = (0.0, 0.0, 0.0)  # (FloatingOrigin keeps it private: the C++ test starts from a reset frame offset)
        anchor0 = rvec(7.0e5)
        bodies = [rvec(2.0e9) for _ in range(nbodies)]
        vessels = []
        for i in range(4):
            vessels.append({"dead": i > 0 and R.random() < 0.2, "pos": active if i == 0 else rvecf(2400.0), "comd": rvec(2500.0)})
        # decision (FloatingOrigin.cs:131-140)
        continuous = (cur_rate > max_phys and mode_high and not landed) or dot(ship_vel, ship_vel) > VEL_FOR_CONTINUOUS_SQR
        flag = not is_zero(out_of_frame)
        flag2 = can_engage and (continuous or sqr_mag_f(active) > THRESHOLD_SQR)
        consumed = (0.0, 0.0, 0.0)
        frame_offset = frame_offset0
        if can_engage and flag:
            frame_offset = add(frame_offset0, out_of_frame)
            consumed = out_of_frame
        offset = (0.0, 0.0, 0.0)
        reverse = (0.0, 0.0, 0.0)
        onk = (0.0, 0.0, 0.0)
        anchor = anchor0
        res_bodies = bodies
        res_vessels = [{"pos": v["pos"], "comd": v["comd"]} for v in vessels]
        if flag2:
            offset = active  # (Vector3d)transform.position
            reverse = (0.0 - offset[0], 0.0 - offset[1], 0.0 - offset[2])
            shift = add(offset, (0.0, 0.0, 0.0))  # offsetNonKrakensbane = offset + nonFrame (nonFrame 0 in kspcore)
            res_bodies = [sub(b, shift) for b in bodies]
            res_vessels = []
            for v in vessels:
                if v["dead"]:
                    res_vessels.append({"pos": v["pos"], "comd": v["comd"]})
                    continue
                # unloaded: v2 = offsetNonKrakensbane; SetPosition(transform.position - v2); CoMD -= v2 (offsets stay below 1e4 m:
                # the CoMRecalcOffsetMaxSqr branch is not taken)
                res_vessels.append({"pos": store3(sub(v["pos"], shift)), "comd": sub(v["comd"], shift)})
            anchor = sub(anchor0, shift)
            onk = add(offset, consumed)
        elif can_engage and flag:
            onk = consumed
        out.append({
            "active": F3(active), "can_engage": can_engage, "landed": landed, "mode_high": mode_high, "cur_rate": hf(cur_rate),
            "max_phys": hf(max_phys), "ship_vel": F3(ship_vel), "out_of_frame": H3(out_of_frame), "frame_offset0": H3(frame_offset0),
            "anchor0": H3(anchor0), "bodies": [H3(b) for b in bodies],
            "vessels": [{"dead": v["dead"], "pos": F3(v["pos"]), "comd": H3(v["comd"])} for v in vessels],
            "shifted": flag2, "continuous": continuous, "offset": H3(offset), "reverse": H3(reverse), "onk": H3(onk),
            "frame_offset": H3(frame_offset), "anchor": H3(anchor), "out_bodies": [H3(b) for b in res_bodies],
            "out_vessels": [{"pos": F3(v["pos"]), "comd": H3(v["comd"]), "com": F3(store3(v["comd"]))} for v in res_vessels],
        })
    return out


# ------------------------------------------------------------------------------------------- Krakensbane with the registry (the host)
MAXV_SQR = 5.0 * 5.0
EXTRA_ALT = 100.0
ALT_THRESHOLD = 200.0
ALT_THRESHOLD_ALONE = 50.0


def krakensbane_scenes(n):
    out = []
    for _ in range(n):
        count = R.choice([1, 2, 3])
        active_i = R.randrange(count)
        fdt = F(0.02)
        vessels = []
        for i in range(count):
            vessels.append({
                "packed": R.random() < 0.15,
                "dead": R.random() < 0.08,
                "landed": R.random() < 0.15,
                "radar": R.choice([R.uniform(0.0, 120.0), R.uniform(140.0, 320.0), R.uniform(500.0, 90000.0)]),
                "vel": rvecf(R.choice([3.0, 40.0, 2300.0])),
                "kacc": rvec(50.0),
            })
        act = vessels[active_i]
        up = rvec(1.0)
        n_up = dot(up, up) ** 0.5
        up = (up[0] / n_up, up[1] / n_up, up[2] / n_up)
        velocity_d = rvec(R.choice([2.0, 50.0, 2300.0]))
        rb_velocity_d = rvec(R.choice([1.0, 30.0]))
        frame_vel0 = (0.0, 0.0, 0.0) if R.random() < 0.4 else rvec(2200.0)

        frame_vel = frame_vel0
        excess = (0.0, 0.0, 0.0)
        last_corr = (0.0, 0.0, 0.0)
        out_of_frame = (0.0, 0.0, 0.0)
        vel = [v["vel"] for v in vessels]
        kacc = [v["kacc"] for v in vessels]

        def change_world_velocity(i, dv):
            v = vessels[i]
            if v["dead"] or v["packed"]:
                return
            vel[i] = store3(add(vel[i], dv))
            kacc[i] = add(kacc[i], mul(dv, 1.0 / fdt))

        # SafeToEngage (Krakensbane.cs:87)
        safe_fo = False
        flag = False
        if not act["dead"]:
            safe_fo = True
            decided = False
            for index in range(count - 1, -1, -1):
                v = vessels[index]
                if index == active_i or (not v["landed"] and v["radar"] >= ALT_THRESHOLD):
                    continue
                safe_fo = False
                decided = True
                break
            if not decided:
                if act["landed"]:
                    flag = False
                else:
                    num = act["radar"] - EXTRA_ALT
                    flag = (not (dot(velocity_d, neg(up)) * fdt > num)) and num >= (ALT_THRESHOLD if count > 1 else ALT_THRESHOLD_ALONE)
        can_engage = safe_fo
        # FixedUpdate (Krakensbane.cs:65-84)
        if not act["packed"]:
            if flag and dot(velocity_d, velocity_d) > MAXV_SQR:
                excess = rb_velocity_d  # AddExcess
                last_corr = neg(excess)
                frame_vel = add(frame_vel, excess)
                for index in range(count - 1, -1, -1):
                    if not vessels[index]["packed"] and not vessels[index]["dead"]:
                        change_world_velocity(index, neg(excess))
            elif not is_zero(frame_vel):
                last_corr = frame_vel  # Zero
                for i in range(count):
                    v = vessels[i]
                    if not v["packed"] and not v["dead"] and (not v["landed"] or i == active_i):
                        change_world_velocity(i, frame_vel)
                frame_vel = (0.0, 0.0, 0.0)
        else:
            frame_vel = (0.0, 0.0, 0.0)
            excess = (0.0, 0.0, 0.0)
        if not is_zero(frame_vel):
            out_of_frame = mul(frame_vel, fdt)
        out.append({
            "active": active_i, "fdt": hf(fdt), "up": H3(up), "velocity_d": H3(velocity_d), "rb_velocity_d": H3(rb_velocity_d),
            "frame_vel0": H3(frame_vel0),
            "vessels": [{"packed": v["packed"], "dead": v["dead"], "landed": v["landed"], "radar": hx(v["radar"]), "vel": F3(v["vel"]),
                         "kacc": H3(v["kacc"])} for v in vessels],
            "frame_vel": H3(frame_vel), "excess": H3(excess), "last_corr": H3(last_corr), "out_of_frame": H3(out_of_frame),
            "can_engage": can_engage, "out_vel": [F3(v) for v in vel], "out_kacc": [H3(k) for k in kacc],
        })
    return out


def main():
    data = {
        "rotating_frame": rotating_frame_scenes(300),
        "change_world_velocity": change_world_velocity_scenes(300),
        "floating_origin": floating_origin_scenes(300, 17),
        "krakensbane_host": krakensbane_scenes(400),
    }
    with open(OUT, "w", newline="\n") as f:
        json.dump(data, f, separators=(",", ":"))
        f.write("\n")
    print("wrote", OUT, os.path.getsize(OUT), "bytes")


if __name__ == "__main__":
    main()
