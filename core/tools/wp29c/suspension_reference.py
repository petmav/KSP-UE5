#!/usr/bin/env python3
"""WP-29c: independent reference for the native WheelCollider (the PhysX 4.1 suspension / tyre law under KSP's wheels) -- test data generator.

Written from the PhysX 4.1 source (NVIDIAGameWorks/PhysX branch 4.1, physx/source/physxvehicle/src/PxVehicleUpdate.cpp:
computeSuspensionRaycast :1109, PxVehicleWheels4SuspensionRaycasts :7255, intersectRayPlane :2273, processSuspTireWheels :3203-3625,
computeTireDirs :1946, computeTireSlips :1962, computeSign :1196, integrateNoDriveWheelSpeeds :4389, computeNoDriveBrakeTorques :1844;
PxVehicleWheels.cpp:54 gMinLongSlipDenominator) and the Unity -> PhysX mapping documented in core/include/ksp/wheels/native_wheel_collider.hpp
(UNVERIFIED Unity glue: the same reading as the C++). It shares no code with the C++ (core/src/wheels/native_wheel_collider.cpp): the C++ was
written from the same PhysX statements, so the two agree only where both transcribe them correctly.

Float rule: PhysX is float32 per operation (numpy float32 scalars below, every constant wrapped). PhysX's CRT float transcendentals (atanf,
sinf, cosf) are modelled as the double function rounded to float (math.atan / sin / cos here; dotnet::Math in the C++). Unity's native
Transform maths (TransformPoint, up / right / forward) are RotateVectorByQuat in float32 (Runtime/Math/Quaternion.h), as in WP-29a.

Every case is synthetic: a random WheelCollider (radius, travel, target, spring, damper, sprung mass, force point, motor / brake torque, steer
angle, friction curves: KSP's all-zero ones or random), a random pose, chassis and previous PhysX state, and a ground plane placed at a chosen
jounce (below max droop, inside the travel, past full compression), tilted, sometimes rejected (distance 0, normal along the travel, ray start
behind the plane), on static ground or on a dynamic / kinematic actor. The outputs are every value of ComputeNativeWheelGeometry and
SimulateNativeWheel, plus a table of friction-curve evaluations.

Usage: python -I core/tools/wp29c/suspension_reference.py > core/tests/data/wp29c_reference.json
"""
import json
import math
import sys

import numpy as np

np.seterr(all="ignore")
F = np.float32
INF = float("inf")
FLT_MAX = F(3.4028234663852886e38)
NO_JOUNCE = FLT_MAX  # PX_MAX_F32

ZERO = F(0.0)
ONE = F(1.0)
TWO = F(2.0)


# ------------------------------------------------------------------------------------------------------------------ PxVec3 (float32 per op)
def v3(a):
    return (F(a[0]), F(a[1]), F(a[2]))


def vadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vmul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def vneg(a):
    return (-a[0], -a[1], -a[2])


def vdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vcross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def vnormalize(v):
    m = F(math.sqrt(float(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])))  # sqrtf: correctly rounded
    if m > ZERO:
        f = ONE / m
        return (v[0] * f, v[1] * f, v[2] * f)
    return v


def pxmax(a, b):
    return b if a < b else a


def pxmin(a, b):
    return a if a < b else b


def compute_sign(f):
    return (ZERO if -f >= ZERO else ONE) if f >= ZERO else F(-1.0)


def atanf(x):
    return F(math.atan(float(x)))


def sinf(x):
    return F(math.sin(float(x)))


def cosf(x):
    return F(math.cos(float(x)))


# ----------------------------------------------------------------------------------------------------------- Unity native Transform maths
def rotate(q, v):
    """RotateVectorByQuat (Runtime/Math/Quaternion.h), float32 per operation."""
    qx, qy, qz, qw = F(q[0]), F(q[1]), F(q[2]), F(q[3])
    x, y, z = qx * TWO, qy * TWO, qz * TWO
    xx, yy, zz = qx * x, qy * y, qz * z
    xy, xz, yz = qx * y, qx * z, qy * z
    wx, wy, wz = qw * x, qw * y, qw * z
    vx, vy, vz = F(v[0]), F(v[1]), F(v[2])
    rx = (ONE - (yy + zz)) * vx + (xy - wz) * vy + (xz + wy) * vz
    ry = (xy + wz) * vx + (ONE - (xx + zz)) * vy + (yz - wx) * vz
    rz = (xz - wy) * vx + (yz + wx) * vy + (ONE - (xx + yy)) * vz
    return (rx, ry, rz)


def transform_point(pos, q, p):
    r = rotate(q, p)
    return (r[0] + F(pos[0]), r[1] + F(pos[1]), r[2] + F(pos[2]))


# ----------------------------------------------------------------------------------------------------------- the model
DEG2RAD = F(0.0174532924)
RPM = F(60.0) / (F(2.0) * F(3.14159265358979))
MIN_LAT_SPEED = F(1.0)
MIN_LONG_SLIP_DEN = F(4.0)
LONG_SLIP_OFFSET = F(0.1) * F(1.0)


def friction_curve(c, slip):
    """Unity's WheelFrictionCurve (documented shape; the section shapes are the model's): see native_wheel_collider.cpp."""
    es, ev, as_, av = F(c["es"]), F(c["ev"]), F(c["as"]), F(c["av"])
    if not (slip > ZERO):
        return ZERO
    if slip < es:
        u = slip / es
        return ev * (u * (TWO - u))
    if slip < as_:
        u = (slip - es) / (as_ - es)
        h = u * u * (F(3.0) - TWO * u)
        return ev + (av - ev) * h
    return av


def geometry(wc, pose):
    top = transform_point(pose["pos"], pose["rot"], wc["center"])
    up = rotate(pose["rot"], (0.0, 1.0, 0.0))
    right = rotate(pose["rot"], (1.0, 0.0, 0.0))
    fwd = rotate(pose["rot"], (0.0, 0.0, 1.0))
    w = vneg(up)
    r = F(wc["radius"])
    D = F(wc["D"])
    t = F(wc["t"])
    g = {"top": top, "up": up, "right": right, "forward": fwd, "w": w, "radius": r}
    g["maxDroop"] = D * t
    g["maxCompression"] = D * (ONE - t)
    g["spring"] = F(wc["spring"])
    g["damper"] = F(wc["damper"])
    g["sprungMass"] = F(wc["sprungMass"])
    g["mass"] = F(wc["mass"])
    g["moi"] = F(0.5) * F(wc["mass"]) * r * r
    g["dampingRate"] = F(wc["damping"])
    centre_rest = vadd(top, vmul(w, g["maxCompression"]))
    g["centreRest"] = centre_rest
    g["rayOrigin"] = vsub(centre_rest, vmul(w, r + g["maxCompression"]))
    length = r + g["maxCompression"] + g["maxDroop"] + r
    length = length + r
    g["rayLength"] = length
    base = vadd(centre_rest, vmul(w, r))
    g["forceAppPoint"] = vadd(base, vmul(up, F(wc["fapd"])))
    return g


def tire_dirs(lat_dir, n, steer):
    tz = vcross(lat_dir, n)
    tx = vcross(n, tz)
    tz = vnormalize(tz)
    tx = vnormalize(tx)
    c = cosf(steer)
    s = sinf(steer)
    long_dir = vadd(vmul(tz, c), vmul(tx, s))
    lat_dir2 = vsub(vmul(tx, c), vmul(tz, s))
    return long_dir, lat_dir2


def tire_slips(long_speed, lat_speed, omega, radius, accel, brake):
    long_abs = abs(long_speed)
    wheel_speed = omega * radius
    wheel_abs = abs(wheel_speed)
    lat_slip = atanf(lat_speed / (long_abs + MIN_LAT_SPEED))
    if long_speed == ZERO and omega == ZERO:
        return ZERO, lat_slip
    if brake or accel:
        long_slip = (wheel_speed - long_speed) / (pxmax(long_abs, wheel_abs) + LONG_SLIP_OFFSET)
    else:
        long_slip = (wheel_speed - long_speed) / pxmax(MIN_LONG_SLIP_DEN, pxmax(long_abs, wheel_abs))
    return long_slip, lat_slip


def simulate(wc, g, q, chassis, prev, dt, gravity):
    o = {}
    recip = ONE / F(dt)
    jounce = -g["maxDroop"]
    o["jounce"] = jounce
    o["grounded"] = False
    for k in ("dx", "jounceSpeed", "suspensionForce", "limitError", "longSpeed", "latSpeed", "longSlip", "latSlip", "tireLongForce",
              "tireLatForce", "tireTorque"):
        o[k] = ZERO
    for k in ("force", "forcePoint", "limitDir", "limitPoint", "wheelBottom", "hitActorForce", "hitActorForcePoint", "tireLongDir",
              "tireLatDir", "tireForce", "hitPoint", "hitNormal"):
        o[k] = (ZERO, ZERO, ZERO)
    o["limitActive"] = False
    o["hitActorForceValid"] = False
    n = v3(q["normal"])
    w = g["w"]
    r = g["radius"]
    motor = F(wc["motor"])
    brake = F(wc["brake"])
    max_accel = abs(motor)
    max_brake = abs(brake)
    intention = max_accel > ZERO and ZERO == max_brake
    is_brake = brake != ZERO
    is_accel = intention and motor != ZERO
    tire_torque = ZERO
    if q["hit"] and F(q["distance"]) != ZERO and vdot(n, w) < ZERO:
        hit_d = -vdot(n, v3(q["point"]))
        v = g["rayOrigin"]
        success = True
        dx = ZERO
        bottom = (ZERO, ZERO, ZERO)
        if n[0] * v[0] + n[1] * v[1] + n[2] * v[2] + hit_d < ZERO:
            success = False
        else:
            T = -(vdot(n, v) + hit_d) / vdot(n, w)
            rest_t = TWO * r + g["maxCompression"]
            dx = rest_t - T
            bottom = vadd(v, vmul(w, rest_t - dx))
        o["dx"] = dx
        if dx > -g["maxDroop"] and success:
            o["wheelBottom"] = bottom
            o["limitError"] = vdot(w, n) * (-dx + g["maxCompression"])
            o["limitActive"] = bool(dx > g["maxCompression"])
            o["limitDir"] = w
            o["limitPoint"] = g["centreRest"]
            jounce = pxmin(dx, g["maxCompression"])
            o["jounce"] = jounce
            rr = vsub(bottom, v3(chassis["com"]))
            wbv = vadd(v3(chassis["vel"]), vcross(v3(chassis["angvel"]), rr))
            dyn = q["actor"] in ("dynamic", "kinematic")
            if dyn:
                hav = vadd(v3(q["actorVel"]), vcross(v3(q["actorAngVel"]), vsub(bottom, v3(q["actorCom"]))))
                wbv = vsub(wbv, hav)
            if F(prev["jounce"]) != NO_JOUNCE:
                js = (jounce - F(prev["jounce"])) * recip
            else:
                js = ZERO
            o["jounceSpeed"] = js
            grav = v3(gravity)
            gsd = vdot(grav, w)
            alpha = pxmax(ZERO, gsd)
            ttb = vsub(grav, vmul(w, alpha)) if ZERO != alpha else (ZERO, ZERO, ZERO)
            fw = pxmax(ZERO, g["sprungMass"] * alpha + g["spring"] * jounce)
            fw = fw + js * g["damper"]
            mag = vdot(n, vsub(vmul(vneg(w), fw), vmul(ttb, g["sprungMass"])))
            if q["actor"] == "dynamic":
                inv_m = ONE / F(q["actorMass"])
                m_back = ONE / inv_m
                sign = compute_sign(mag)
                fm = abs(mag)
                ca = pxmin(fm * inv_m, FLT_MAX)
                cfm = ca * m_back * sign
                mag = cfm
                o["hitActorForceValid"] = True
                o["hitActorForce"] = vmul(n, -cfm * ONE)
                o["hitActorForcePoint"] = v3(q["point"])
            o["suspensionForce"] = mag
            o["force"] = vmul(n, mag)
            o["forcePoint"] = g["forceAppPoint"]
            o["grounded"] = True
            long_dir, lat_dir = tire_dirs(g["right"], n, F(wc["steer"]) * DEG2RAD)
            o["tireLongDir"] = long_dir
            o["tireLatDir"] = lat_dir
            o["longSpeed"] = vdot(wbv, long_dir)
            o["latSpeed"] = vdot(wbv, lat_dir)
            o["longSlip"], o["latSlip"] = tire_slips(o["longSpeed"], o["latSpeed"], F(prev["wheelSpeed"]), r, is_accel, is_brake)
            load = mag
            if load > ZERO:
                fc, sc = wc["fwd"], wc["side"]
                lc = friction_curve(fc, abs(o["longSlip"]))
                sc_v = friction_curve(sc, abs(o["latSlip"]))
                o["tireLongForce"] = compute_sign(o["longSlip"]) * lc * F(fc["stiff"]) * load
                o["tireLatForce"] = -(compute_sign(o["latSlip"]) * sc_v * F(sc["stiff"]) * load)
                tire_torque = -o["tireLongForce"] * r
                o["tireTorque"] = tire_torque
                o["tireForce"] = vadd(vmul(long_dir, o["tireLongForce"]), vmul(lat_dir, o["tireLatForce"]))
            o["hitPoint"] = v3(q["point"])
            o["hitNormal"] = n
    old = F(prev["wheelSpeed"])
    brake_t = -compute_sign(old) * brake
    sub = F(dt) * (ONE / g["moi"])
    res = (old + sub * (tire_torque + motor + brake_t)) / (ONE + g["dampingRate"] * sub)
    if is_brake and (old * res <= ZERO):
        res = ZERO
    o["wheelSpeed"] = res
    o["rpm"] = res * RPM
    return o


# ----------------------------------------------------------------------------------------------------------- the cases
class Gen:
    def __init__(self, seed):
        self.r = np.random.RandomState(seed)

    def u(self, a, b):
        return float(F(self.r.uniform(a, b)))

    def vec(self, s):
        return [self.u(-s, s) for _ in range(3)]

    def unit(self):
        while True:
            v = self.r.normal(0, 1, 3)
            n = float(np.linalg.norm(v))
            if n > 0.1:
                return v / n

    def quat(self, tilt_max_deg):
        """A rotation: random yaw about Y, then a tilt about a random horizontal axis (float32 components, renormalised in double)."""
        yaw = self.r.uniform(-math.pi, math.pi)
        tilt = math.radians(self.r.uniform(0.0, tilt_max_deg))
        ax = self.r.uniform(-math.pi, math.pi)
        qy = np.array([0.0, math.sin(yaw / 2), 0.0, math.cos(yaw / 2)])
        h = (math.cos(ax), 0.0, math.sin(ax))
        qt = np.array([h[0] * math.sin(tilt / 2), 0.0, h[2] * math.sin(tilt / 2), math.cos(tilt / 2)])

        def qmul(a, b):
            ax_, ay, az, aw = a
            bx, by, bz, bw = b
            return np.array([aw * bx + ax_ * bw + ay * bz - az * by, aw * by - ax_ * bz + ay * bw + az * bx,
                             aw * bz + ax_ * by - ay * bx + az * bw, aw * bw - ax_ * bx - ay * by - az * bz])

        q = qmul(qt, qy)
        q = q / np.linalg.norm(q)
        return [float(F(x)) for x in q]


def curve(g, ksp):
    if ksp:
        return {"es": 0.0, "ev": 0.0, "as": 0.0, "av": 0.0, "stiff": 0.0}
    es = g.u(0.05, 1.5)
    return {"es": es, "ev": g.u(0.3, 2.0), "as": float(F(es + g.u(0.1, 3.0))), "av": g.u(0.1, 1.5), "stiff": g.u(0.0, 2.0)}


def make_case(g, idx):
    ksp = idx % 3 != 2  # two thirds with KSP's all-zero friction curves
    wc = {
        "center": g.vec(0.3) if idx % 4 else [0.0, 0.0, 0.0],
        "radius": g.u(0.1, 1.0),
        "D": g.u(0.02, 1.2),
        "t": g.u(0.1, 1.0) if idx % 7 else 1.0,
        "spring": g.u(0.1, 5000.0),
        "damper": g.u(0.1, 500.0),
        "sprungMass": g.u(0.01, 2.0),
        "mass": 1.0 if ksp else g.u(0.2, 30.0),
        "fapd": g.u(-0.5, 1.5),
        "motor": [0.0, 1e-5, 0.0, g.u(-50.0, 50.0)][idx % 4] if not ksp else [0.0, float(F(1e-5))][idx % 2],
        "brake": 0.0 if (ksp or idx % 5) else g.u(0.0, 40.0),
        "steer": g.u(-40.0, 40.0) if idx % 3 else 0.0,
        "damping": 0.25 if idx % 6 else g.u(0.05, 2.0),
        "fwd": curve(g, ksp),
        "side": curve(g, ksp),
    }
    wc["motor"] = float(F(wc["motor"]))
    pose = {"pos": g.vec(60.0), "rot": g.quat(60.0 if idx % 5 else 10.0)}
    chassis = {"com": g.vec(60.0), "vel": g.vec(25.0) if idx % 3 else g.vec(0.2), "angvel": g.vec(2.0) if idx % 4 else [0.0, 0.0, 0.0]}
    prev = {"jounce": float(NO_JOUNCE) if idx % 6 == 0 else 0.0, "wheelSpeed": g.u(-60.0, 60.0) if idx % 5 else 0.0}
    dt = float(F([0.02, 0.04, 0.06, 0.08, 0.005][idx % 5]))
    gravity = [0.0, 0.0, 0.0] if idx % 9 else g.vec(10.0)
    geo = geometry(wc, pose)
    if prev["jounce"] == 0.0:
        prev["jounce"] = float(F(g.u(-1.0, 1.0)) * (geo["maxDroop"] + geo["maxCompression"]))
    # the ground: a plane through the point of the suspension line at a chosen jounce, tilted
    kind = idx % 10
    q = {"hit": False, "distance": 0.0, "point": [0.0, 0.0, 0.0], "normal": [0.0, 1.0, 0.0], "actor": "none", "actorMass": 0.0,
         "actorVel": [0.0, 0.0, 0.0], "actorAngVel": [0.0, 0.0, 0.0], "actorCom": [0.0, 0.0, 0.0]}
    if kind != 0:
        md, mc = float(geo["maxDroop"]), float(geo["maxCompression"])
        if kind == 1:
            j = g.u(-md - 0.4, -md - 1e-4)          # below max droop: in the air although the ray hits
        elif kind == 2:
            j = float(F(mc + g.u(1e-4, 0.3)))       # past full compression: the travel limit
        else:
            j = g.u(-md, mc)                        # inside the travel
        r = float(geo["radius"])
        T = 2.0 * r + mc - j
        o = [float(x) for x in geo["rayOrigin"]]
        wv = [float(x) for x in geo["w"]]
        p = [o[i] + wv[i] * T for i in range(3)]
        up = [-x for x in wv]
        tilt = math.radians(g.u(0.0, 35.0))
        side = g.unit()
        side = side - np.dot(side, up) * np.array(up)
        side = side / np.linalg.norm(side)
        nrm = np.array(up) * math.cos(tilt) + side * math.sin(tilt)
        nrm = nrm / np.linalg.norm(nrm)
        q["hit"] = True
        q["point"] = [float(F(x)) for x in p]
        q["normal"] = [float(F(x)) for x in nrm]
        q["distance"] = float(F(T))
        if kind == 3:
            q["distance"] = 0.0                     # the ray started inside the shape
        elif kind == 4:
            q["normal"] = [float(F(-x)) for x in nrm]  # facing along the travel
        elif kind == 5:
            # the ray start is behind the plane: move the plane above the start point
            q["point"] = [float(F(o[i] - wv[i] * 0.05)) for i in range(3)]
        if kind in (6, 7, 8):
            q["actor"] = ["dynamic", "kinematic", "static"][kind - 6]
            if kind != 8:
                q["actorMass"] = g.u(0.01, 50.0)
                q["actorVel"] = g.vec(5.0)
                q["actorAngVel"] = g.vec(1.0)
                q["actorCom"] = [float(F(p[i] + g.u(-3.0, 3.0))) for i in range(3)]
    out = simulate(wc, geo, q, chassis, prev, dt, gravity)
    return {"wc": wc, "pose": pose, "chassis": chassis, "prev": prev, "dt": dt, "gravity": gravity, "query": q, "geo": geo, "out": out}


def jf(x):
    x = float(x)
    if x != x:
        return "nan"
    if x == INF:
        return "inf"
    if x == -INF:
        return "-inf"
    assert float(F(x)) == x, "value %r is not float32-exact" % x
    if x == 0.0 and math.copysign(1.0, x) < 0:
        return "-0"
    return x


def conv(o):
    if isinstance(o, dict):
        return {k: conv(v) for k, v in o.items()}
    if isinstance(o, (list, tuple)):
        return [conv(v) for v in o]
    if isinstance(o, (bool, np.bool_)):
        return bool(o)
    if isinstance(o, str):
        return o
    if isinstance(o, (int,)) and not isinstance(o, bool):
        return o
    return jf(o)


def main():
    g = Gen(2903)
    cases = [make_case(g, i) for i in range(2000)]
    curves = []
    for i in range(400):
        c = curve(g, False)
        if i % 10 == 0:
            c["es"] = 0.0
        s = g.u(0.0, 5.0) if i % 7 else [0.0, -0.5, float(F(c["es"])), float(F(c["as"]))][i % 4]
        curves.append({"curve": c, "slip": float(F(s)), "value": friction_curve(c, F(s))})
    stats = {
        "cases": len(cases),
        "grounded": sum(1 for c in cases if c["out"]["grounded"]),
        "limit": sum(1 for c in cases if c["out"]["limitActive"]),
        "actorForce": sum(1 for c in cases if c["out"]["hitActorForceValid"]),
        "tireForce": sum(1 for c in cases if c["out"]["tireLongForce"] != 0 or c["out"]["tireLatForce"] != 0),
        "noJounce": sum(1 for c in cases if c["prev"]["jounce"] == float(NO_JOUNCE)),
    }
    json.dump(conv({"source": "tools/wp29c/suspension_reference.py", "seed": 2903, "stats": stats, "cases": cases, "curves": curves}),
              sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
