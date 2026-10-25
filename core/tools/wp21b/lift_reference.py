#!/usr/bin/env python3
"""WP-21b: independent double-precision reference for the lifting / control surfaces in the flight bubble's module slot.

Reads the dump `tests/test_scenery_scenarios.cpp` ("WP-21b Aeris 3A: full-throttle roll ...") writes when KSPCORE_AIRCRAFT_DUMP is set: for every
sampled physics tick and every ModuleLiftingSurface / ModuleControlSurface of the Aeris 3A, the kinematic state the slot read (the part transform,
its rigid body's velocity / angular velocity / centre of mass, Krakensbane's frame velocity, the dynamic pressure and Mach number of the part, the
vessel's CurrentCoM and ReferenceTransform, the control state) and the module's outputs (point velocity, lift and drag force, deflection, the
potential torque pair). From the STATE it recomputes the OUTPUTS from the KSP source, in float64, with the lifting-surface curves and multipliers
read from harness/inputs/Physics.cfg and the module parameters read from the part .cfg of the GameData mirror (cross-checked against the
values the C++ module held):

  ModuleLiftingSurface.cs:205 SetupCoefficients, :232 GetLiftVector, :262 GetDragVector (the Default lifting-surface curves of Physics.cfg,
  Unity AnimationCurve cubic Hermite, clamped), :170 FixedUpdate; ModuleControlSurface.cs:366 FixedUpdate, :704 FixedCtrlSurfaceUpdate (the
  action / roll mixing, MoveTowards at actuatorSpeed * fixedDeltaTime), :469 GetPotentialLift, :476 GetPotentialTorque.

It shares no code with the C++ (core/src/aero/lifting_surface.cpp, core/src/physics/part_forces.cpp). The C++ is float32 with the Mono rule, the
reference float64: the comparison tolerance is stated per quantity below. It also evaluates the aircraft's vertical force balance at the
sampled ticks (the lifting surfaces' lift and drag from THIS reference, the engine's thrust and the weight from the dump; the fuselage's body
lift / drag, the gear and the wings' unmodelled interaction are not in it) and brackets the lift-off against the weight.

No capture covers an aircraft flight: every number here is analytic / reference-gated, not a KSP measurement.

Usage: python -I core/tools/wp21b/lift_reference.py --states <dump.json> [--json out.json]
Exit status 0 when every gate holds.
"""
import argparse
import json
import math
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
PHYSICS_CFG = ROOT / "harness" / "inputs" / "Physics.cfg"
PARTS = ROOT / "core" / "testdata" / "GameData-mirror" / "GameData" / "Squad" / "Parts"

# tolerances (relative to the larger of the value's magnitude and a floor; float32 evaluation vs float64)
TOL_VEL = 2e-4        # point velocity
TOL_FORCE = 2e-4      # lift / drag force vectors, potential torque (relative to max(|F|, FORCE_FLOOR))
FORCE_FLOOR = 5e-3    # kN
TOL_DEFLECTION = 2e-4  # degrees, absolute (plus 1e-5 relative)
TOL_ACTION = 2e-4
TOL_LIFTOFF = 0.05    # lift-off speed predicted from the reference's surfaces + thrust vs the simulation
TOL_CLIMB = 0.05      # vertical speed 10 s after the start of the rotation, integrated from the reference's vertical force vs the simulation


# ---------------------------------------------------------------------------------------------------------------- config parsing
def strip_comments(text):
    return "\n".join(l.split("//")[0] for l in text.splitlines())


def block(text, name, start=0):
    """the body of the first `name { ... }` at or after `start` (brace matched) and the end index"""
    m = re.compile(r"(?m)^\s*" + re.escape(name) + r"\s*\{").search(text, start)
    if not m:
        return None, -1
    depth, i = 1, m.end()
    while depth and i < len(text):
        depth += (text[i] == "{") - (text[i] == "}")
        i += 1
    return text[m.end():i - 1], i


def curve(body):
    keys = []
    for m in re.finditer(r"(?m)^\s*key\s*=\s*(\S+)\s+(\S+)\s*(\S+)?\s*(\S+)?", body):
        t, v = float(m.group(1)), float(m.group(2))
        keys.append((t, v, float(m.group(3)) if m.group(3) else 0.0, float(m.group(4)) if m.group(4) else 0.0))
    return keys


def evaluate(keys, x):
    """Unity AnimationCurve.Evaluate with cubic Hermite segments and clamped ends (FloatCurve)"""
    if not keys:
        return 0.0
    if x <= keys[0][0]:
        return keys[0][1]
    if x >= keys[-1][0]:
        return keys[-1][1]
    for a, b in zip(keys, keys[1:]):
        if a[0] <= x <= b[0]:
            dt = b[0] - a[0]
            s = (x - a[0]) / dt
            h00 = 2 * s ** 3 - 3 * s ** 2 + 1
            h10 = s ** 3 - 2 * s ** 2 + s
            h01 = -2 * s ** 3 + 3 * s ** 2
            h11 = s ** 3 - s ** 2
            return h00 * a[1] + h10 * dt * a[3] + h01 * b[1] + h11 * dt * b[2]
    return keys[-1][1]


def physics_cfg():
    text = strip_comments(PHYSICS_CFG.read_text(encoding="utf-8-sig", errors="replace"))
    out = {"liftMultiplier": float(re.search(r"(?m)^\s*liftMultiplier\s*=\s*(\S+)", text).group(1)),
           "liftDragMultiplier": float(re.search(r"(?m)^\s*liftDragMultiplier\s*=\s*(\S+)", text).group(1)), "curves": {}}
    body, _ = block(text, "LIFTING_SURFACE_CURVES")
    pos = 0
    while True:
        b, end = block(body, "LIFTING_SURFACE", pos)
        if b is None:
            break
        name = re.search(r"(?m)^\s*name\s*=\s*(\S+)", b).group(1)
        out["curves"][name] = {k: curve(block(b, k)[0]) for k in ("lift", "liftMach", "drag", "dragMach")}
        pos = end
    return out


_part_cache = {}


def part_cfg(name):
    """the PART's own fields and its MODULE blocks (name -> dict of key = value) from the GameData mirror"""
    if name in _part_cache:
        return _part_cache[name]
    for cfg in PARTS.rglob("*.cfg"):
        text = strip_comments(cfg.read_text(encoding="utf-8-sig", errors="replace"))
        m = re.search(r"(?m)^\s*name\s*=\s*" + re.escape(name.replace(".", "_")) + r"\s*$", text) or re.search(
            r"(?m)^\s*name\s*=\s*" + re.escape(name) + r"\s*$", text)
        if not m or not re.search(r"(?m)^\s*PART\b", text):
            continue
        mods = {}
        for mm in re.finditer(r"(?ms)^\s*MODULE\s*\{(.*?)^\s*\}", text):
            kv = dict(re.findall(r"(?m)^\s*(\w+)\s*=\s*(.+?)\s*$", mm.group(1)))
            if "name" in kv and kv["name"] not in mods:
                mods[kv["name"]] = kv
        _part_cache[name] = mods
        return mods
    _part_cache[name] = {}
    return {}


# ---------------------------------------------------------------------------------------------------------------- vector maths
def add(a, b): return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
def sub(a, b): return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
def mul(a, s): return [a[0] * s, a[1] * s, a[2] * s]
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
def norm(a): return math.sqrt(dot(a, a))
def normalized(a):
    n = norm(a)
    return [a[0] / n, a[1] / n, a[2] / n] if n > 1e-5 else [0.0, 0.0, 0.0]  # Vector3.normalized (kEpsilon 1e-5)
def scale(a, b): return [a[0] * b[0], a[1] * b[1], a[2] * b[2]]


def qmul_vec(q, v):
    x, y, z, w = q
    u = [x, y, z]
    t = mul(cross(u, v), 2.0)
    return add(add(v, mul(t, w)), cross(u, t))


def qinv(q): return [-q[0], -q[1], -q[2], q[3]]


def axis_angle(deg, axis):
    a = normalized(axis)
    h = math.radians(deg) * 0.5
    s = math.sin(h)
    return [a[0] * s, a[1] * s, a[2] * s, math.cos(h)]


def sign(x): return 1.0 if x >= 0.0 else -1.0  # Mathf.Sign


def clamp(x, lo, hi): return max(lo, min(hi, x))


def move_towards(cur, target, delta):
    return target if abs(target - cur) <= delta else cur + sign(target - cur) * delta


# ---------------------------------------------------------------------------------------------------------------- the law
class Surface:
    def __init__(self, rec, phys):
        self.r = rec
        c = phys["curves"][rec["curve"]]
        self.lift, self.liftMach, self.drag, self.dragMach = c["lift"], c["liftMach"], c["drag"], c["dragMach"]
        self.liftMul, self.dragMul = phys["liftMultiplier"], phys["liftDragMultiplier"]
        self.Q = rec["dynamicPressurekPa"] * 1000.0

    def axes(self):
        q = self.r["partRot"]
        return qmul_vec(q, [1, 0, 0]), qmul_vec(q, [0, 1, 0]), qmul_vec(q, [0, 0, 1])

    def point_velocity(self):
        r = self.r
        w = r["rbAngularVelocity"]
        return add(add(r["rbVelocity"], cross(w, sub(r["partPos"], r["rbCoM"]))), r["frameVelocity"])

    def coefficients(self, pv, lift_vector=None):
        right, up, fwd = self.axes()
        d = self.r["transformDir"]  # X = 0, Y = 1, Z = 2
        lv = fwd if d == 2 else (up if d == 1 else right)
        lv = mul(lv, self.r["transformSign"]) if lift_vector is None else lift_vector
        n = normalized(pv)
        ld = dot(n, lv)
        ad = abs(ld) if self.r["omnidirectional"] else clamp(ld, 0.0, 1.0)
        return n, lv, ld, ad

    def lift_vec(self, lv, ld, ad, mach):
        s = sign(ld) * evaluate(self.lift, ad) * evaluate(self.liftMach, mach) * self.r["deflectionLiftCoeff"]
        if s == 0.0 or s != s:
            return [0.0, 0.0, 0.0]
        s = self.Q * (self.liftMul * s)
        return mul(lv, -s)

    def drag_vec(self, n, ad, mach):
        s = evaluate(self.drag, ad) * evaluate(self.dragMach, mach) * self.r["deflectionLiftCoeff"]
        if s == 0.0 or s != s:
            return [0.0, 0.0, 0.0]
        s = self.Q * (s * self.dragMul)
        return mul(n, -s)

    def forces(self):
        """(pointVelocity, liftForce, dragForce, action, deflection) of ModuleLiftingSurface.FixedUpdate / ModuleControlSurface.FixedUpdate"""
        r = self.r
        pv = self.point_velocity()
        mach = r["machNumber"]
        n, lv, ld, ad = self.coefficients(pv)
        if r["module"] != "ModuleControlSurface":
            lift = self.lift_vec(lv, ld, ad, mach)
            drag = self.drag_vec(n, ad, mach) if r["useInternalDragModel"] else [0, 0, 0]
            return pv, lift, drag, None, None, None
        area = r["ctrlSurfaceArea"]
        base_lift = self.lift_vec(lv, ld, ad, mach)
        lift = mul(base_lift, 1.0 - area)
        drag = mul(self.drag_vec(n, ad, mach), 1.0 - area) if r["useInternalDragModel"] else [0, 0, 0]
        action, deflection = self.control(pv)
        # `ctrlSurface.localRotation != neutral`: Unity's approximate quaternion equality (dot > 1 - 1e-6)
        if not math.cos(math.radians(deflection) * 0.5) > 1.0 - 1e-6:
            q = axis_angle(deflection, qmul_vec(self.r["partRot"], [1, 0, 0]))
            lv2 = qmul_vec(q, lv)
            f = dot(n, lv2)
            a2 = abs(f)
        else:
            lv2, f, a2 = lv, ld, ad
        lift = add(lift, mul(self.lift_vec(lv2, f, a2, mach), area))
        if r["useInternalDragModel"]:
            drag = add(drag, mul(self.drag_vec(n, a2, mach), area))
        return pv, lift, drag, action, deflection, base_lift

    def control(self, pv):
        """ModuleControlSurface.FixedCtrlSurfaceUpdate (:704), the fixed (non rotor) branch"""
        r = self.r
        q = r["partRot"]
        right, up, _ = self.axes()
        local = qmul_vec(qinv(q), sub(r["currentCoM"], r["partPos"]))  # baseTransform.InverseTransformPoint(CoM)
        rhs = [local[0], 0.0, local[2]]
        pitch, yaw, roll = r["ctrl"]
        ip, iy, ir = r["ignore"]
        inp = qmul_vec(r["refRot"], [0.0 if ip else pitch, 0.0, 0.0 if iy else yaw])
        action = dot(inp, right)
        rng = r["ctrlSurfaceRange"]
        if ir:
            rl = 0.0
        else:
            ref_up = qmul_vec(r["refRot"], [0, 1, 0])
            rl = dot([1, 0, 0], rhs) * (1.0 - (abs(dot(normalized(rhs), qmul_vec(qinv(q), ref_up))) * 0.5 + 0.5)) * sign(dot(up, ref_up)) * roll * sign(rng)
        if local[1] < 0.0:
            action = -action
        action = 1.0 * clamp(action - rl, -1.0, 1.0)
        action = rng * action * r["authorityLimiter"] * 0.01
        assert not r["deploy"], "deploy is not used by the scenario"
        action = clamp(action, -1.5 * rng, 1.5 * rng)
        step = r["actuatorSpeed"] * r["fixedDeltaTime"]
        assert not r["useExponentialSpeed"]
        return action, move_towards(r["prevDeflection"], action, step)

    def potential_torque(self, base_lift, n):
        """ModuleControlSurface.GetPotentialTorque (:476): (pos, neg) in the reference transform's axes"""
        r = self.r
        ig = r["ignore"]
        if self.Q == 0.0 or all(ig):
            return [0, 0, 0], [0, 0, 0]
        vec = sub(r["rbCoM"], r["currentCoM"])
        iref = qinv(r["refRot"])
        b = qmul_vec(iref, vec)
        q = r["partRot"]
        area = r["ctrlSurfaceArea"]
        _, _, fwd = self.axes()
        right = qmul_vec(q, [1, 0, 0])
        mach = r["machNumber"]

        def pot(positive):
            num = 1.0 if positive else -1.0
            ang = r["currentDeployAngle"] + num * r["ctrlSurfaceRange"] * r["authorityLimiter"] * 0.01
            rhs = qmul_vec(axis_angle(ang, right), fwd)
            f = dot(n, rhs)
            return sub(mul(self.lift_vec(rhs, f, abs(f), mach), area), mul(base_lift, area))

        out = []
        for positive in (True, False):
            t = scale(qmul_vec(iref, cross(vec, pot(positive))), b)
            out.append(t)
        pos, neg = out
        if ig[0]:
            pos[0] = neg[0] = 0.0
        if ig[2]:
            pos[1] = neg[1] = 0.0
        else:
            pos[1], neg[1] = -pos[1], -neg[1]
        if ig[1]:
            pos[2] = neg[2] = 0.0
        return pos, neg


def rel_err(a, b, floor):
    return max(abs(x - y) for x, y in zip(a, b)) / max(norm(b), floor)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--states", required=True)
    ap.add_argument("--json", default=None)
    args = ap.parse_args()
    doc = json.loads(pathlib.Path(args.states).read_text())
    phys = physics_cfg()
    res = {"records": len(doc["records"]), "maxRel": {}, "failures": []}
    worst = {"pointVelocity": 0.0, "lift": 0.0, "drag": 0.0, "action": 0.0, "deflection": 0.0, "potentialTorque": 0.0}
    cfg_mismatch = []
    by_tick = {}
    nonzero_deflection = 0
    for rec in doc["records"]:
        s = Surface(rec, phys)
        pv, lift, drag, action, deflection, base_lift = s.forces()
        n, _, _, _ = s.coefficients(pv)
        worst["pointVelocity"] = max(worst["pointVelocity"], rel_err(pv, rec["pointVelocity"], 1.0))
        worst["lift"] = max(worst["lift"], rel_err(lift, rec["liftForce"], FORCE_FLOOR))
        worst["drag"] = max(worst["drag"], rel_err(drag, rec["dragForce"], FORCE_FLOOR))
        if action is not None:
            worst["action"] = max(worst["action"], abs(action - rec["action"]))
            worst["deflection"] = max(worst["deflection"], abs(deflection - rec["deflection"]) / max(1.0, abs(deflection)))
            nonzero_deflection += 1 if abs(rec["deflection"]) > 1e-3 else 0
            pos, neg = s.potential_torque(base_lift, n)
            for a, b in ((pos, rec["potentialTorque"][0]), (neg, rec["potentialTorque"][1])):
                worst["potentialTorque"] = max(worst["potentialTorque"], max(abs(x - y) for x, y in zip(a, b)) / max(norm(b), 1e-3))
        # the module parameters the C++ held vs the part .cfg (the loader's values)
        mods = part_cfg(rec["part"])
        m = mods.get("ModuleControlSurface") if action is not None else mods.get("ModuleLiftingSurface")
        if m is not None:
            for key, field in (("deflectionLiftCoeff", "deflectionLiftCoeff"), ("ctrlSurfaceRange", "ctrlSurfaceRange"), ("ctrlSurfaceArea", "ctrlSurfaceArea"),
                               ("actuatorSpeed", "actuatorSpeed")):
                if key in m and field in rec and abs(float(m[key]) - rec[field]) > 1e-6 * max(1.0, abs(rec[field])):
                    cfg_mismatch.append((rec["part"], key, m[key], rec[field]))
        by_tick.setdefault(rec["tick"], []).append((rec, lift, drag))
    gates = [("pointVelocity", TOL_VEL), ("lift", TOL_FORCE), ("drag", TOL_FORCE), ("action", TOL_ACTION), ("deflection", TOL_DEFLECTION), ("potentialTorque", 5e-3)]
    ok = True
    for k, tol in gates:
        res["maxRel"][k] = worst[k]
        if not worst[k] <= tol:
            ok = False
            res["failures"].append("%s %.3g > %.3g" % (k, worst[k], tol))
    if cfg_mismatch:
        ok = False
        res["failures"].append("module parameters differ from the part cfg: %r" % cfg_mismatch[:3])
    res["recordsWithDeflection"] = nonzero_deflection

    # ---- vertical force balance of the aircraft from this reference's surfaces + the dump's thrust and weight ------------------------------
    MU = 3.5316e12
    rows = []
    for e in doc["series"]:
        t = e["tick"]
        if t not in by_tick:
            continue
        up = e["up"]
        fz = 0.0
        for rec, lift, drag in by_tick[t]:
            fz += dot(add(lift, drag), up)
        tz = e["thrust"] * dot(e["thrustDir"], up)
        weight = e["mass"] * MU / e["radius"] ** 2
        rows.append({"t": e["t"], "grounded": e["grounded"], "speed": e["srfSpeed"], "vz": e["verticalSpeed"], "surfaceLiftOverWeight": fz / weight,
                     "netOverWeight": (fz + tz - weight) / weight, "weight": weight})
    res["balance"] = rows
    last_grounded = [r for r in rows if r["grounded"] > 0][-1] if any(r["grounded"] > 0 for r in rows) else None
    first_air = next((r for r in rows if r["grounded"] == 0), None)
    ev = doc["events"]
    res["liftoff"] = {"sim": {"t": ev["liftoffT"], "speed": ev["liftoffSpeed"]}, "lastGrounded": last_grounded, "firstAirborne": first_air}
    if first_air is None or last_grounded is None:
        ok = False
        res["failures"].append("no lift-off in the dump")
    # the unstick speed predicted from the law: the speed at which the reference's surfaces + thrust balance the weight at the sampled attitude
    # (linear interpolation of the net / weight between the samples around the sim's lift-off)
    if first_air is not None and last_grounded is not None and first_air["netOverWeight"] != last_grounded["netOverWeight"]:
        f = (0.0 - last_grounded["netOverWeight"]) / (first_air["netOverWeight"] - last_grounded["netOverWeight"])
        pred = last_grounded["speed"] + f * (first_air["speed"] - last_grounded["speed"])
        res["liftoff"]["predictedSpeed"] = pred
        res["liftoff"]["predictedVsSim"] = pred / ev["liftoffSpeed"] - 1.0
        if abs(res["liftoff"]["predictedVsSim"]) > TOL_LIFTOFF:
            ok = False
            res["failures"].append("predicted lift-off speed %.2f vs sim %.2f (%.1f %% > %.0f %%)" % (pred, ev["liftoffSpeed"], 100 * res["liftoff"]["predictedVsSim"], 100 * TOL_LIFTOFF))
    # ---- the residual force (body aero, gear, interpolation) over the climb: m dvz/dt - (surfaces + thrust - weight) --------------------------
    resid = []
    for a, b, c in zip(rows, rows[1:], rows[2:]):
        if a["grounded"] or b["grounded"] or c["grounded"]:
            continue
        ent = [e for e in doc["series"] if abs(e["t"] - b["t"]) < 1e-9][0]
        az = (c["vz"] - a["vz"]) / (c["t"] - a["t"])
        resid.append({"t": b["t"], "residualOverWeight": (ent["mass"] * az / b["weight"] * 1.0) - b["netOverWeight"]})
    res["climbResidual"] = {"samples": len(resid), "maxAbs": max((abs(r["residualOverWeight"]) for r in resid), default=0.0),
                            "mean": sum(r["residualOverWeight"] for r in resid) / max(1, len(resid))}
    # ---- the climb rate 10 s after the start of the rotation, integrated from the reference's vertical force balance (trapezoid over the samples) -
    air = [r for r in rows if r["grounded"] == 0]
    if air:
        t_end = ev["rotateT"] + 10.0
        seg = [r for r in air if r["t"] <= t_end + 1e-9]
        mass_of = {e["t"]: e["mass"] for e in doc["series"]}
        vz = seg[0]["vz"]
        for r0, r1 in zip(seg, seg[1:]):
            a0 = r0["netOverWeight"] * r0["weight"] / mass_of[r0["t"]]
            a1 = r1["netOverWeight"] * r1["weight"] / mass_of[r1["t"]]
            vz += 0.5 * (a0 + a1) * (r1["t"] - r0["t"])
        res["climb"] = {"tEnd": seg[-1]["t"], "predictedVz": vz, "simVz": seg[-1]["vz"], "relative": vz / seg[-1]["vz"] - 1.0}
        if abs(res["climb"]["relative"]) > TOL_CLIMB:
            ok = False
            res["failures"].append("climb rate predicted %.2f vs sim %.2f m/s" % (vz, seg[-1]["vz"]))
    # ---- VesselSAS's torque sum: the control surfaces' share (with - without the torque provider hook) = the sum of GetPotentialTorque ----------
    st = doc.get("sasTorque")
    if st:
        pos_sum, neg_sum = [0.0] * 3, [0.0] * 3
        for rec, lift, drag in by_tick.get(st["tick"], []):
            if rec["module"] != "ModuleControlSurface":
                continue
            sf = Surface(rec, phys)
            pv, _, _, _, _, base_lift = sf.forces()
            p, n_ = sf.potential_torque(base_lift, sf.coefficients(pv)[0])
            pos_sum, neg_sum = add(pos_sum, p), add(neg_sum, n_)
        d_pos = sub(st["with"][0], st["without"][0])
        d_neg = sub(st["with"][1], st["without"][1])
        # the SAS call happens after the tick (the surfaces moved by one tick): 2 % of the norm
        e = max(max(abs(a - b) for a, b in zip(d_pos, pos_sum)) / max(norm(pos_sum), 1e-3), max(abs(a - b) for a, b in zip(d_neg, neg_sum)) / max(norm(neg_sum), 1e-3))
        res["sasTorque"] = {"tick": st["tick"], "surfacesPos": pos_sum, "withMinusWithoutPos": d_pos, "surfacesNeg": neg_sum, "withMinusWithoutNeg": d_neg, "maxRel": e}
        if not e < 0.02:
            ok = False
            res["failures"].append("SAS torque sum: control surface share differs from the reference by %.3g" % e)
    else:
        ok = False
        res["failures"].append("no SAS torque sample in the dump")
    if res["recordsWithDeflection"] < 50:
        ok = False
        res["failures"].append("too few deflected control-surface samples")
    res["ok"] = ok
    if args.json:
        pathlib.Path(args.json).write_text(json.dumps(res, indent=1))
    summary = {k: (v if not isinstance(v, list) else "...") for k, v in res.items()}
    print(json.dumps(summary, indent=1))
    for f in res["failures"]:
        print("FAIL:", f, file=sys.stderr)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
