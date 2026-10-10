#!/usr/bin/env python3
"""WP-40: independent reference for the editor maths -- test data generator for tests/test_editor.cpp.

Re-implements, in plain Python (numpy float32) and from the decompiled C#, the numeric core of
  Transform.localPosition / localRotation after EditorLogic.attachPart (EditorLogic.cs:3835: attPos0 / attRotation0)
  EditorLogic.UpdateSymmetry radial + mirror (EditorLogic.cs:3315) and UpdateChildMirrorSymmetry (:3391)
  EditorGeometryUtil.MirrorPos / MirrorDirection / MirrorRotation (EditorGeometryUtil.cs:77-100)
  GizmoOffset.SnapToGrid / OnHandleMove, GizmoRotate.SnapToGrid / OnHandleRotate (EditorGizmos/*.cs)
  EditorMarker_CoM.findCenterOfMass (EditorMarker_CoM.cs:35) and EditorMarker_CoT.FindCoT (EditorMarker_CoT.cs:42)
  the PreFlightTests predicates that depend only on part modules / resources / tree / stages / crew-less manifests
It shares NO code with kspcore. The Mono float rule is followed: managed float expressions are evaluated in double and rounded to float32
where the C# stores / passes / returns a float; native Unity code (AngleAxis, LookRotation, Euler, eulerAngles, Normalize on set) is
per-operation float32 (sinf / cosf / asin / atan2 are evaluated in double and rounded to float32, which is the correctly rounded value).

Inputs (original game data, never hand-authored):
  harness/inputs/Ships/{VAB,SPH}/*.craft and analysis/physics/craft/*.craft  (the stock crafts)
  harness/captures/out2/part_database.json  (part masses, CoMOffset, resources, modules, attach nodes, cfg values)

Usage: python -I core/tools/wp40/editor_reference.py > core/tests/data/wp40_reference.json
"""
import glob
import json
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
F = np.float32


def f(x):
    return F(x)


def bits(x):
    return int(np.asarray(F(x)).view(np.uint32))


def from_hex(h):
    return F(np.asarray(int(h, 16), dtype=np.uint32).view(np.float32))


def parse_float(s):
    return F(float(s))


# ---------------------------------------------------------------------------------------------------------------------------------------
# managed Vector3 / Quaternion (Mono rule: double expression, rounded where the C# stores / passes)
# ---------------------------------------------------------------------------------------------------------------------------------------
def v3(x, y, z):
    return (F(x), F(y), F(z))


ZERO = v3(0, 0, 0)
UP = v3(0, 1, 0)
FORWARD = v3(0, 0, 1)
RIGHT = v3(1, 0, 0)
ONE = v3(1, 1, 1)


def vadd(a, b):
    return tuple(f(float(x) + float(y)) for x, y in zip(a, b))


def vsub(a, b):
    return tuple(f(float(x) - float(y)) for x, y in zip(a, b))


def vneg(a):
    return tuple(F(-float(x)) for x in a)


def vmul(a, s):
    return tuple(f(float(x) * float(s)) for x in a)


def vdiv(a, s):
    return tuple(f(float(x) / float(s)) for x in a)


def vscale(a, b):
    return tuple(f(float(x) * float(y)) for x, y in zip(a, b))


def dot(a, b):
    return f(float(a[0]) * float(b[0]) + float(a[1]) * float(b[1]) + float(a[2]) * float(b[2]))


def sqr_mag(a):
    return f(float(a[0]) * float(a[0]) + float(a[1]) * float(a[1]) + float(a[2]) * float(a[2]))


def mag(a):
    return f(math.sqrt(float(a[0]) * float(a[0]) + float(a[1]) * float(a[1]) + float(a[2]) * float(a[2])))


def vnormalized(a):
    num = mag(a)
    if float(num) > float(F(1e-05)):
        return vdiv(a, num)
    return ZERO


def project_on_plane(vec, n):
    num = dot(n, n)
    if float(num) < 1.401298e-45:
        return vec
    num2 = dot(vec, n)
    return tuple(f(float(vec[i]) - float(n[i]) * float(num2) / float(num)) for i in range(3))


def vangle(a, b):
    num = f(math.sqrt(float(sqr_mag(a)) * float(sqr_mag(b))))
    if float(num) < float(F(1e-15)):
        return F(0)
    num2 = max(-1.0, min(1.0, float(f(float(dot(a, b)) / float(num)))))
    return f(float(f(math.acos(num2))) * float(F(57.29578)))


def qmul(l, r):
    lx, ly, lz, lw = (float(x) for x in l)
    rx, ry, rz, rw = (float(x) for x in r)
    return (f(lw * rx + lx * rw + ly * rz - lz * ry), f(lw * ry + ly * rw + lz * rx - lx * rz),
            f(lw * rz + lz * rw + lx * ry - ly * rx), f(lw * rw - lx * rx - ly * ry - lz * rz))


def qmul_vec(q, p):
    x, y, z, w = (float(c) for c in q)
    num = float(f(x * 2.0))
    num2 = float(f(y * 2.0))
    num3 = float(f(z * 2.0))
    n4 = float(f(x * num))
    n5 = float(f(y * num2))
    n6 = float(f(z * num3))
    n7 = float(f(x * num2))
    n8 = float(f(x * num3))
    n9 = float(f(y * num3))
    n10 = float(f(w * num))
    n11 = float(f(w * num2))
    n12 = float(f(w * num3))
    px, py, pz = (float(c) for c in p)
    return (f((1.0 - (n5 + n6)) * px + (n7 - n12) * py + (n8 + n11) * pz), f((n7 + n12) * px + (1.0 - (n4 + n6)) * py + (n9 - n10) * pz),
            f((n8 - n11) * px + (n9 + n10) * py + (1.0 - (n4 + n5)) * pz))


def conj(q):
    return (F(-q[0]), F(-q[1]), F(-q[2]), q[3])


QID = (F(0), F(0), F(0), F(1))


# PartTransform (the Part's Transform stand-in; localScale is (1,1,1) for every craft part)
def t_point(t, p):
    return vadd(qmul_vec(t["rot"], vscale(t["scale"], p)), t["pos"])


def t_inv_point(t, p):
    r = qmul_vec(conj(t["rot"]), vsub(p, t["pos"]))
    return tuple(f(float(r[i]) / float(t["scale"][i])) for i in range(3))


def t_dir(t, d):
    return qmul_vec(t["rot"], d)


def t_inv_dir(t, d):
    return qmul_vec(conj(t["rot"]), d)


def t_up(t):
    return qmul_vec(t["rot"], UP)


def t_forward(t):
    return qmul_vec(t["rot"], FORWARD)


# ---------------------------------------------------------------------------------------------------------------------------------------
# native Unity quaternion functions (per-operation float32)
# ---------------------------------------------------------------------------------------------------------------------------------------
# sinf / cosf of Unity native code are the UCRT's single-precision functions (not always the correctly rounded value): call ucrtbase itself when
# it is there (Windows; the reference file is generated there once), else the correctly rounded value
try:
    import ctypes

    _UCRT = ctypes.CDLL("ucrtbase")
    _UCRT.sinf.restype = ctypes.c_float
    _UCRT.sinf.argtypes = [ctypes.c_float]
    _UCRT.cosf.restype = ctypes.c_float
    _UCRT.cosf.argtypes = [ctypes.c_float]
except (OSError, AttributeError):
    _UCRT = None


def sinf(x):
    return F(_UCRT.sinf(float(x))) if _UCRT else F(math.sin(float(x)))


def cosf(x):
    return F(_UCRT.cosf(float(x))) if _UCRT else F(math.cos(float(x)))


def angle_axis(angle, axis):
    rad = F(F(F(F(angle) / F(360.0)) * F(2.0)) * F(3.14159265358979323846))
    m = np.sqrt(F(F(F(axis[0] * axis[0]) + F(axis[1] * axis[1])) + F(axis[2] * axis[2])))
    if m > F(0.000001):
        half = F(rad * F(0.5))
        w = cosf(half)
        s = F(sinf(half) / m)
        return (F(s * axis[0]), F(s * axis[1]), F(s * axis[2]), w)
    return QID


def matrix_to_quaternion(m):
    trace = F(F(m[0][0] + m[1][1]) + m[2][2])
    q = [F(0)] * 4
    if trace > F(0):
        root = np.sqrt(F(trace + F(1)))
        q[3] = F(F(0.5) * root)
        root = F(F(0.5) / root)
        q[0] = F(F(m[2][1] - m[1][2]) * root)
        q[1] = F(F(m[0][2] - m[2][0]) * root)
        q[2] = F(F(m[1][0] - m[0][1]) * root)
    else:
        nxt = [1, 2, 0]
        i = 0
        if m[1][1] > m[0][0]:
            i = 1
        if m[2][2] > m[i][i]:
            i = 2
        j = nxt[i]
        k = nxt[j]
        root = np.sqrt(F(F(F(m[i][i] - m[j][j]) - m[k][k]) + F(1)))
        q[i] = F(F(0.5) * root)
        root = F(F(0.5) / root)
        q[3] = F(F(m[k][j] - m[j][k]) * root)
        q[j] = F(F(m[j][i] + m[i][j]) * root)
        q[k] = F(F(m[k][i] + m[i][k]) * root)
    mg = np.sqrt(F(F(F(F(q[0] * q[0]) + F(q[1] * q[1])) + F(q[2] * q[2])) + F(q[3] * q[3])))
    return tuple(F(c / mg) for c in q)


def fcross(a, b):
    return (F(F(a[1] * b[2]) - F(a[2] * b[1])), F(F(a[2] * b[0]) - F(a[0] * b[2])), F(F(a[0] * b[1]) - F(a[1] * b[0])))


def fmag(a):
    return np.sqrt(F(F(F(a[0] * a[0]) + F(a[1] * a[1])) + F(a[2] * a[2])))


def look_rotation(forward, up):
    eps = F(0.00001)
    z = [F(c) for c in forward]
    m = fmag(z)
    if m < eps:
        return QID
    z = [F(c / m) for c in z]
    x = list(fcross(up, z))
    m = fmag(x)
    if m < eps:
        return QID
    x = [F(c / m) for c in x]
    y = fcross(z, x)
    sq = F(F(F(y[0] * y[0]) + F(y[1] * y[1])) + F(y[2] * y[2]))
    if not (abs(F(sq - F(1))) < F(0.000001)):
        return QID
    mat = [[x[r], y[r], z[r]] for r in range(3)]
    return matrix_to_quaternion(mat)


def native_qmul(l, r):
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    x = F(F(F(F(lw * rx) + F(lx * rw)) + F(ly * rz)) - F(lz * ry))
    y = F(F(F(F(lw * ry) + F(ly * rw)) + F(lz * rx)) - F(lx * rz))
    z = F(F(F(F(lw * rz) + F(lz * rw)) + F(lx * ry)) - F(ly * rx))
    w = F(F(F(F(lw * rw) - F(lx * rx)) - F(ly * ry)) - F(lz * rz))
    return (x, y, z, w)


def euler(deg):
    d2r = F(0.0174532924)
    e = [F(c * d2r) for c in deg]
    c = [cosf(F(a / F(2.0))) for a in e]
    s = [sinf(F(a / F(2.0))) for a in e]
    qx = (s[0], F(0), F(0), c[0])
    qy = (F(0), s[1], F(0), c[1])
    qz = (F(0), F(0), s[2], c[2])
    return native_qmul(native_qmul(qy, qx), qz)


def euler_angles(q):
    x, y, z, w = q
    x2, y2, z2 = F(x * F(2)), F(y * F(2)), F(z * F(2))
    xx, yy, zz = F(x * x2), F(y * y2), F(z * z2)
    xy, xz, yz = F(x * y2), F(x * z2), F(y * z2)
    wx, wy, wz = F(w * x2), F(w * y2), F(w * z2)
    m00 = F(F(1) - F(yy + zz))
    m01 = F(xy - wz)
    m02 = F(xz + wy)
    m10 = F(xy + wz)
    m11 = F(F(1) - F(xx + zz))
    m12 = F(yz - wx)
    m22 = F(F(1) - F(xx + yy))
    pi = F(3.14159265358979323846)
    if m12 < F(0.999):
        if m12 > F(-0.999):
            vx = F(math.asin(float(F(-m12))))
            vy = F(math.atan2(float(m02), float(m22)))
            vz = F(math.atan2(float(m10), float(m11)))
        else:
            vx = F(pi * F(0.5))
            vy = F(math.atan2(float(m01), float(m00)))
            vz = F(0)
    else:
        vx = F(F(-pi) * F(0.5))
        vy = F(math.atan2(float(F(-m01)), float(m00)))
        vz = F(0)
    e = [F(c * F(57.29578)) for c in (vx, vy, vz)]
    lo = F(-0.005729578)
    hi = F(F(360) + lo)
    out = []
    for c in e:
        if c < lo:
            c = F(c + F(360))
        elif c > hi:
            c = F(c - F(360))
        out.append(c)
    return tuple(out)


def normalize_safe(q):
    m = np.sqrt(F(F(F(F(q[0] * q[0]) + F(q[1] * q[1])) + F(q[2] * q[2])) + F(q[3] * q[3])))
    if m < F(0.00001):
        return QID
    return tuple(F(c / m) for c in q)


def managed_normalize(q):
    """Quaternion.Normalize (Quaternion.cs:302): Mathf.Sqrt(Dot(q, q)), identity under Mathf.Epsilon, each component divided (Mono rule)"""
    d = f(float(q[0]) * float(q[0]) + float(q[1]) * float(q[1]) + float(q[2]) * float(q[2]) + float(q[3]) * float(q[3]))
    num = f(math.sqrt(float(d)))
    if float(num) < 1.401298e-45:
        return QID
    return tuple(f(float(c) / float(num)) for c in q)


def mathf_round(x):
    # Math.Round(double) banker's rounding of the float, back to float
    return F(np.round(np.float64(float(x))))


# ---------------------------------------------------------------------------------------------------------------------------------------
# data: part database + crafts
# ---------------------------------------------------------------------------------------------------------------------------------------
def load_part_db():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "part_database.json")))
    tabs = {}
    for t in d["tables"]:
        cols = [c["name"] if isinstance(c, dict) else c for c in t["columns"]]
        tabs[t["name"]] = [dict(zip(cols, r)) for r in t["rows"]]
    db = {"parts": {}, "resources": {}, "modules": {}, "fields": {}, "props": {}, "nodes": {}, "cfg": {}}
    for r in tabs["parts"]:
        db["parts"][r["name"]] = r
    for r in tabs["resources"]:
        db["resources"].setdefault(r["part"], []).append(r)
    for r in tabs["modules"]:
        db["modules"].setdefault(r["part"], []).append(r)
    for r in tabs["module_fields"]:
        db["fields"].setdefault((r["part"], r["module_index"]), {})[r["field"]] = r
    for r in tabs["module_propellants"]:
        db["props"].setdefault((r["part"], r["module_index"]), []).append(r)
    for r in tabs["attach_nodes"]:
        db["nodes"].setdefault(r["part"], []).append(r)
    for r in tabs["part_cfg_values"]:
        db["cfg"].setdefault(r["part"], {}).setdefault(r["key"], []).append(r["cfg_value"])
    return db


def parse_craft(path):
    parts = []
    stack = []
    cur = None
    mod = None
    res = None
    lines = open(path, encoding="latin-1").read().split(chr(10))
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if line == "{":
            continue
        if line == "}":
            top = stack.pop()
            if top == "MODULE":
                mod = None
            elif top == "RESOURCE":
                res = None
            continue
        if "=" in line and not line.endswith("{"):
            k, _, v = line.partition("=")
            k = k.strip()
            v = v.strip()
            if stack == ["PART"]:
                if k in ("link", "sym", "attN", "srfN"):
                    cur.setdefault(k, []).append(v)
                else:
                    cur[k] = v
            elif stack == ["PART", "MODULE"] and mod is not None:
                mod.setdefault(k, v)
            elif stack == ["PART", "RESOURCE"] and res is not None:
                res.setdefault(k, v)
            continue
        # a node header
        stack.append(line)
        if stack == ["PART"]:
            cur = {"modules": [], "resources": []}
            parts.append(cur)
        elif stack == ["PART", "MODULE"]:
            mod = {}
            cur["modules"].append(mod)
        elif stack == ["PART", "RESOURCE"]:
            res = {}
            cur["resources"].append(res)
    return parts


def craft_files():
    out = []
    for d in ("VAB", "SPH"):
        for p in sorted(glob.glob(os.path.join(ROOT, "harness", "inputs", "Ships", d, "*.craft"))):
            out.append((d + "/" + os.path.basename(p), p))
    for p in sorted(glob.glob(os.path.join(ROOT, "analysis", "physics", "craft", "*.craft"))):
        out.append(("physics/" + os.path.basename(p), p))
    return out


def split_id(token):
    name, _, num = token.rpartition("_")
    return name, int(num)


def floats(s):
    return [parse_float(x) for x in s.replace("|", ",").split(",")]


def craft_type(path):
    for line in open(path, encoding="latin-1").read().split(chr(10)):
        if line.startswith("type ="):
            return line.split("=", 1)[1].strip()
        if line.strip() == "PART":
            break
    return ""


def build_craft(label, path, db):
    raw = parse_craft(path)
    default_method = "Mirror" if craft_type(path) == "SPH" else "Radial"
    parts = []
    for i, r in enumerate(raw):
        name, cid = split_id(r["part"])
        parts.append({"i": i, "name": name, "id": cid, "raw": r, "pos": tuple(floats(r["pos"])), "rot": tuple(floats(r["rot"])),
                      "parent": -1, "children": [], "sym": [], "label": label, "attm": int(r.get("attm", "0")), "symMethod": r.get("symMethod", default_method)})
    by_id = {}
    for p in parts:
        by_id[p["id"]] = p["i"]            # GetPartByCraftID: the last match
    for p in parts:
        for ln in p["raw"].get("link", []):
            c = by_id.get(split_id(ln)[1])
            if c is not None:
                parts[c]["parent"] = p["i"]
                p["children"].append(c)
        for sn in p["raw"].get("sym", []):
            c = by_id.get(split_id(sn)[1])
            p["sym"].append(c if c is not None else -1)
    return parts


def transform(p):
    return {"pos": p["pos"], "rot": p["rot"], "scale": ONE}


# ---------------------------------------------------------------------------------------------------------------------------------------
# sections
# ---------------------------------------------------------------------------------------------------------------------------------------
def sec_local_pose(parts):
    out = []
    for p in parts:
        if p["parent"] < 0:
            out.append(None)
            continue
        par = parts[p["parent"]]
        out.append({"attPos0": [bits(c) for c in t_inv_point(transform(par), p["pos"])],
                    "attRot0": [bits(c) for c in qmul(conj(par["rot"]), p["rot"])]})
    return out


def part_nodes_orientation(db, name, partner_id, parts, p):
    """orientation of the node of part p (a child) that is linked to its parent: the attach node table of the prefab, or the surface attach
    node (cfg node_attach = position, orientation) when the part is surface attached (Part.FindAttachNodeByPart also returns srfAttachNode)"""
    srf = p["raw"].get("srfN")
    if srf is not None:
        fields = srf[0].split(",") if isinstance(srf, list) else srf.split(",")
        if len(fields) > 1 and fields[1] and split_id(fields[1])[1] == partner_id:
            na = db["cfg"].get(name, {}).get("node_attach")
            if na:
                vals = [x.strip() for x in na[0].split(",")]
                return v3(parse_float(vals[3]), parse_float(vals[4]), parse_float(vals[5]))
            return None
    for an in p["raw"].get("attN", []):
        nid, _, rest = an.partition(",")
        if split_id(rest.split("_")[0] + "_" + rest.split("_")[1])[1] == partner_id:
            for n in db["nodes"].get(name, []):
                if n["id"] == nid:
                    return v3(from_hex(n["orientation_x_hex"]), from_hex(n["orientation_y_hex"]), from_hex(n["orientation_z_hex"]))
    return None


def first_non_symmetrical_parent(parts, idx):
    while parts[idx]["sym"]:
        idx = parts[idx]["parent"]
    return idx


def in_hierarchy(parts, part, tgt):
    # EditorLogicBase.PartInHierarchy: tgt is a direct or indirect child of part
    if tgt in parts[part]["children"]:
        return True
    return any(in_hierarchy(parts, c, tgt) for c in parts[part]["children"])


class SymState:
    def __init__(self, parts):
        self.pos = [p["pos"] for p in parts]
        self.rot = [p["rot"] for p in parts]

    def t(self, i):
        return {"pos": self.pos[i], "rot": self.rot[i], "scale": ONE}

    def set(self, i, pos, rot):
        self.pos[i] = pos
        self.rot[i] = normalize_safe(rot)


def update_symmetry(parts, db, st, sel, root, sym_mode, part_parent, method, coord_space):
    selp = parts[sel]
    pp_sel = selp["parent"]
    num = 1
    for c in selp["sym"]:
        if c >= 0 and parts[c]["parent"] == pp_sel:
            num += 1
    for i in range(sym_mode):
        ci = selp["sym"][i]
        num2 = (i + 1) % num
        ref = st.t(sel)
        if method == "Mirror":
            part = first_non_symmetrical_parent(parts, part_parent)
            vector2 = vsub(ref["pos"], st.pos[part])
            vector3 = project_on_plane(vector2, t_up(st.t(root)))
            newpos = vadd(vadd(st.pos[part], vsub(vector2, vector3)), qmul_vec(angle_axis(F(180.0), vneg(t_forward(st.t(root)))), vector3))
            direction = t_inv_dir(st.t(root), t_up(ref))
            direction2 = t_inv_dir(st.t(root), t_forward(ref))
            direction = (F(-direction[0]), direction[1], direction[2])
            direction2 = (F(-direction2[0]), direction2[1], direction2[2])
            direction = t_dir(st.t(root), direction)
            direction2 = t_dir(st.t(root), direction2)
            q = look_rotation(direction2, direction)
            if selp["attm"] == 1:
                na = db["cfg"].get(selp["name"], {}).get("node_attach")
                if na:
                    vals = [x.strip() for x in na[0].split(",")]
                    if parse_float(vals[3]) != F(0):
                        q = qmul(angle_axis(F(180.0), direction), q)
            st.set(ci, newpos, q)
            mirror_children(parts, db, st, sel, part, i, root, sel)
        else:
            pp_ci = parts[ci]["parent"]
            if pp_ci == pp_sel:
                angle = f(float(num2) * 360.0 / float(num))
                axis = t_up(st.t(root)) if coord_space == "World" else t_up(st.t(pp_sel))
                q = angle_axis(angle, axis)
                vec = vadd(qmul_vec(q, vsub(ref["pos"], st.pos[pp_sel])), st.pos[pp_ci])
                q2 = qmul(q, ref["rot"])
                st.set(ci, vec, q2)
            elif pp_ci >= 0:
                if num2 == 0:
                    vec = t_point(st.t(pp_ci), t_inv_point(st.t(pp_sel), ref["pos"]))
                    q2 = qmul(st.rot[pp_ci], qmul(conj(st.rot[pp_sel]), ref["rot"]))
                else:
                    prev = selp["sym"][num2 - 1]
                    pprev = parts[prev]["parent"]
                    vec = t_point(st.t(pp_ci), t_inv_point(st.t(pprev), st.pos[prev]))
                    q2 = qmul(st.rot[pp_ci], qmul(conj(st.rot[pprev]), st.rot[prev]))
                st.set(ci, vec, q2)


def mirror_children(parts, db, st, part, mirror_root, sym_idx, root, selected):
    for c in parts[part]["children"]:
        p2 = parts[c]
        if len(p2["sym"]) > sym_idx and p2["sym"][sym_idx] >= 0 and not in_hierarchy(parts, selected, p2["sym"][sym_idx]):
            vec = vsub(st.pos[c], st.pos[mirror_root])
            vec2 = project_on_plane(vec, t_up(st.t(root)))
            position = vadd(vadd(st.pos[mirror_root], vsub(vec, vec2)), qmul_vec(angle_axis(F(180.0), vneg(t_forward(st.t(root)))), vec2))
            direction = t_inv_dir(st.t(root), t_up(st.t(c)))
            direction2 = t_inv_dir(st.t(root), t_forward(st.t(c)))
            direction = (F(-direction[0]), direction[1], direction[2])
            direction2 = (F(-direction2[0]), direction2[1], direction2[2])
            direction = t_dir(st.t(root), direction)
            direction2 = t_dir(st.t(root), direction2)
            rotation = look_rotation(direction2, direction)
            ori = part_nodes_orientation(db, p2["name"], parts[part]["id"], parts, p2)
            if ori is not None and ori[0] != F(0):
                rotation = qmul(angle_axis(F(180.0), direction), rotation)
            cc = p2["sym"][sym_idx]
            st.pos[cc] = position
            st.rot[cc] = normalize_safe(rotation)
            mirror_children(parts, db, st, c, mirror_root, sym_idx, root, selected)


def has_module(db, name, cls):
    return any(m["moduleName"] == cls or m["class"] == cls for m in db["modules"].get(name, []))


def sec_symmetry(parts, db):
    root = next(p["i"] for p in parts if p["parent"] < 0)
    cases = []
    done = 0
    for p in parts:
        if not p["sym"] or p["parent"] < 0 or any(c < 0 for c in p["sym"]):
            continue
        if done >= 8:
            break
        if any(parts[c]["parent"] < 0 for c in p["sym"]):
            continue
        group = [p["i"]] + p["sym"]
        method = p["symMethod"]
        # the counterparts must list the same set (the set is symmetric in the file) and carry the same method
        if any(parts[c]["symMethod"] != method for c in p["sym"]):
            continue
        for method_used, space in (("Radial", "Self"), ("Radial", "World")) if method == "Radial" else (("Mirror", "Self"),):
            if method_used == "Mirror":
                if len(p["sym"]) != 1:
                    continue
                if has_module(db, p["name"], "ModuleMirrorPlacement") or has_module(db, parts[p["sym"][0]]["name"], "ModuleMirrorPlacement"):
                    continue
                # UpdateChildMirrorSymmetry reads the attach nodes of every mirrored child: need their prefab orientation
            st = SymState(parts)
            try:
                update_symmetry(parts, db, st, p["i"], root, len(p["sym"]), p["parent"], method_used, space)
            except Exception:
                continue
            cases.append({"part": p["i"], "method": method_used, "space": space, "mode": len(p["sym"]),
                          "result": [{"part": c, "pos": [bits(x) for x in st.pos[c]], "rot": [bits(x) for x in st.rot[c]]} for c in
                                     sorted(set(i for i in range(len(parts)) if st.pos[i] != parts[i]["pos"] or st.rot[i] != parts[i]["rot"]))]})
        done += 1
    return cases


def sec_mirror_helpers(parts):
    root = next(p["i"] for p in parts if p["parent"] < 0)
    rt = transform(parts[root])
    out = []
    for p in parts[:24]:
        if p["i"] == root:
            continue
        # EditorGeometryUtil.MirrorPos / MirrorDirection / MirrorRotation (mirrorRoot = setRoot = the root part)
        vec = vsub(p["pos"], rt["pos"])
        vec2 = project_on_plane(vec, vmul(t_up(rt), dot(t_up(rt), vec)))
        mpos = vadd(vadd(rt["pos"], vsub(vec, vec2)), qmul_vec(angle_axis(F(180.0), vneg(t_forward(rt))), vec2))
        d = p["pos"]
        d2 = project_on_plane(d, vmul(t_up(rt), dot(t_up(rt), d)))
        mdir = vadd(vsub(d, d2), qmul_vec(angle_axis(F(180.0), vneg(t_forward(rt))), d2))
        a = t_inv_dir(rt, qmul_vec(p["rot"], UP))
        b = t_inv_dir(rt, qmul_vec(p["rot"], FORWARD))
        a = (F(-a[0]), a[1], a[2])
        b = (F(-b[0]), b[1], b[2])
        a = t_dir(rt, a)
        b = t_dir(rt, b)
        mrot = qmul(angle_axis(F(180.0), a), look_rotation(b, a))
        out.append({"part": p["i"], "pos": [bits(x) for x in mpos], "dir": [bits(x) for x in mdir], "rot": [bits(x) for x in mrot]})
    return out


def sec_gizmos(parts):
    out = []
    for p in parts[:16]:
        par = parts[p["parent"]] if p["parent"] >= 0 else None
        host_t = transform(p)
        res = {"part": p["i"], "offset": [], "rotate": []}
        for space in ("Self", "World"):
            for snap in (F(0.2), F(0.1)):
                def rnd(v):
                    return F(mathf_round(f(float(v) / float(snap))) * snap)
                if space == "Self":
                    lp = t_inv_point(transform(par), host_t["pos"]) if par else host_t["pos"]
                    lp2 = (rnd(lp[0]), rnd(lp[1]), rnd(lp[2]))
                    newpos = t_point(transform(par), lp2) if par else lp2
                else:
                    lp = vsub(host_t["pos"], transform(par)["pos"]) if par else host_t["pos"]
                    lp2 = (rnd(lp[0]), rnd(lp[1]), rnd(lp[2]))
                    newpos = vadd(lp2, transform(par)["pos"]) if par else lp2
                res["offset"].append({"space": space, "snap": bits(snap), "pos": [bits(x) for x in newpos]})
        # OnHandleMove: offset = offset0 + axis * distance; gizmo position = trfPos0 + offset (axis = the part's own axes, distance = |pos|)
        for axis in (RIGHT, UP, FORWARD):
            dist = mag(p["pos"])
            off = vadd(ZERO, vmul(axis, dist))
            res["offset"].append({"axis": [bits(x) for x in axis], "distance": bits(dist), "move": [bits(x) for x in vadd(p["pos"], off)]})
        e = euler_angles(p["rot"])
        for snap in (F(15.0), F(5.0)):
            sn = tuple(F(mathf_round(f(float(c) / float(snap))) * snap) for c in e)
            res["rotate"].append({"snap": bits(snap), "euler": [bits(c) for c in e], "snapped": [bits(c) for c in sn],
                                  "rot": [bits(c) for c in normalize_safe(euler(sn))]})
        # OnHandleRotate: gizmo rotation = AngleAxis(dAngle, axis) * rot0 (managed multiply), callback AngleAxis(dAngle, axis)
        for k, axis in enumerate((RIGHT, UP, FORWARD)):
            d_angle = e[k]
            aa = angle_axis(d_angle, axis)
            res["rotate"].append({"axis": [bits(x) for x in axis], "dAngle": bits(d_angle), "aa": [bits(x) for x in aa],
                                  "gizmo": [bits(x) for x in normalize_safe(qmul(aa, p["rot"]))]})
        out.append(res)
    return out


def part_info(db, name):
    return db["parts"].get(name)


def resource_mass(db, p):
    """Part.GetResourceMass: the resources from last to first, amount * density summed in double, rounded to float"""
    base = [(r["name"], float(r["amount"]), from_hex(r["density_hex"])) for r in db["resources"].get(p["name"], [])]
    order = [b[0] for b in base]
    amounts = {b[0]: b[1] for b in base}
    dens = {b[0]: b[2] for b in base}
    for r in p["raw"]["resources"]:
        n = r.get("name")
        if n is None:
            continue
        a = float(r.get("amount", "0"))
        if n not in amounts:
            order.append(n)
            for rows in db["resources"].values():
                for rr in rows:
                    if rr["name"] == n:
                        dens[n] = from_hex(rr["density_hex"])
                        break
                if n in dens:
                    break
        amounts[n] = a
    total = 0.0
    for n in reversed(order):
        total += amounts[n] * float(dens[n])
    return f(total)


def physical_full(info):
    return info["physicalSignificance"] == "FULL"


def sec_com(parts, db):
    root = next(p["i"] for p in parts if p["parent"] < 0)
    com = ZERO
    m = F(0)
    def recurse(i):
        nonlocal com, m
        p = parts[i]
        info = part_info(db, p["name"])
        mass = from_hex(info["mass_hex"])
        rm = resource_mass(db, p)
        w = f(float(mass) + float(rm))
        def at(q):
            qi = part_info(db, q["name"])
            off = v3(from_hex(qi["CoMOffset_x_hex"]), from_hex(qi["CoMOffset_y_hex"]), from_hex(qi["CoMOffset_z_hex"]))
            return vadd(q["pos"], qmul_vec(q["rot"], off))
        if physical_full(info):
            com = vadd(com, vmul(at(p), w))
            m = f(float(m) + (float(mass) + float(rm)))
        elif p["parent"] >= 0:
            com = vadd(com, vmul(at(parts[p["parent"]]), w))
            m = f(float(m) + (float(mass) + float(rm)))
        for c in p["children"]:
            recurse(c)
    recurse(root)
    return {"com": [bits(x) for x in vdiv(com, m)], "mass": bits(m)}


_CFG_CACHE = {}


def cfg_modules(db, name):
    """the MODULE nodes of the part cfg (original game data: core/testdata/GameData-mirror) as {key: value} dicts, in file order"""
    if name in _CFG_CACHE:
        return _CFG_CACHE[name]
    path = os.path.join(ROOT, "core", "testdata", "GameData-mirror", db["parts"][name]["cfg_file"].replace("/", os.sep))
    mods = []
    stack = []
    cur = None
    for raw in open(path, encoding="utf-8-sig", errors="replace").read().split(chr(10)):
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line == "{":
            continue
        if line == "}":
            top = stack.pop()
            if top == "MODULE":
                cur = None
            continue
        if "=" in line:
            k, _, v = line.partition("=")
            if stack == ["PART", "MODULE"] and cur is not None:
                cur.setdefault(k.strip(), v.strip())
            continue
        stack.append(line)
        if stack == ["PART", "MODULE"]:
            cur = {}
            mods.append(cur)
    _CFG_CACHE[name] = mods
    return mods


def module_field_f(db, name, idx, field, default=None):
    cm = cfg_modules(db, name)
    if idx < len(cm) and field in cm[idx]:
        return parse_float(cm[idx][field])
    r = db["fields"].get((name, idx), {}).get(field)
    if r is None:
        return default
    return F(r["value_num"]) if r["type"] == "Single" else r["value"]


def sec_cot(parts, db):
    root = next(p["i"] for p in parts if p["parent"] < 0)
    origin = ZERO
    direction = ZERO
    t = F(0)
    def recurse(i):
        nonlocal origin, direction, t
        p = parts[i]
        mods = db["modules"].get(p["name"], [])
        craft_mods = [m for m in p["raw"]["modules"] if m.get("name") in ("ModuleEngines", "ModuleEnginesFX")]
        # modules are visited last to first (the C# loop is `while (count-- > 0)`)
        for mm in reversed(mods):
            mi = mm["module_index"]
            if mm["class"] not in ("ModuleEngines", "ModuleEnginesFX"):
                continue
            max_thrust = module_field_f(db, p["name"], mi, "maxThrust", F(215.0))
            tp = F(db["fields"][(p["name"], mi)]["thrustPercentage"]["value_num"]) if "thrustPercentage" in db["fields"].get((p["name"], mi), {}) else F(100)
            # thrustPercentage is persistent: the craft MODULE node (same module order) overrides it
            cm = p["raw"]["modules"][mi] if mi < len(p["raw"]["modules"]) and p["raw"]["modules"][mi].get("name") == mm["moduleName"] else {}
            if "thrustPercentage" in cm:
                tp = parse_float(cm["thrustPercentage"])
            if cm.get("manuallyOverridden", "False") == "True":
                continue   # ModuleEngines.OnCenterOfThrustQuery (ModuleEngines.cs:2103) answers nothing while manuallyOverridden
            tpos = p["pos"]
            trot = managed_normalize(qmul(p["rot"], angle_axis(F(90.0), RIGHT)))
            fwd = qmul_vec(trot, FORWARD)
            qpos = vadd(ZERO, vmul(tpos, F(1.0)))
            qdir = vadd(ZERO, vmul(fwd, F(1.0)))
            thrust = f(float(max_thrust) * (float(tp) * float(F(0.01))))
            origin = vadd(origin, vmul(qpos, thrust))
            direction = vadd(direction, vmul(qdir, thrust))
            t = f(float(t) + float(thrust))
        for c in p["children"]:
            recurse(c)
    recurse(root)
    if t != F(0):
        num = f(1.0 / float(t))
        o = vmul(origin, num)
        d = vmul(direction, num)
        dn = vnormalized(d)
        return {"origin": [bits(x) for x in o], "direction": [bits(x) for x in dn], "thrust": bits(t)}
    return {"origin": [0, 0, 0], "direction": [0, 0, 0], "thrust": 0}


# ---------------------------------------------------------------------------------------------------------------------------------------
# pre-flight predicates derived from the craft contents
# ---------------------------------------------------------------------------------------------------------------------------------------
def mods(db, name):
    return [m["moduleName"] for m in db["modules"].get(name, [])]


def classes(db, name):
    return [m["class"] for m in db["modules"].get(name, [])]


def sec_preflight(parts, db):
    root = next(p["i"] for p in parts if p["parent"] < 0)
    res = {}
    names = [p["name"] for p in parts]
    # AirBreathingEnginesForIntakes / IntakesForAirBreathingEngines
    intakes = []
    air_engines = []
    for p in parts:
        for m in db["modules"].get(p["name"], []):
            mi = m["module_index"]
            if m["class"] == "ModuleResourceIntake":
                if db["fields"].get((p["name"], mi), {}).get("resourceName", {}).get("value") == "IntakeAir":
                    intakes.append(p["i"])
            if m["class"] in ("ModuleEngines", "ModuleEnginesFX"):
                if any(r["name"] == "IntakeAir" for r in db["props"].get((p["name"], mi), [])):
                    air_engines.append(p["i"])
    intakes = sorted(set(intakes))
    air_engines = sorted(set(air_engines))
    res["AirBreathingEnginesForIntakes"] = {"pass": not (intakes and not air_engines), "failed": [] if (not intakes or air_engines) else intakes}
    res["IntakesForAirBreathingEngines"] = {"pass": not (air_engines and not intakes), "failed": [] if (not air_engines or intakes) else air_engines}
    # AntennaPresent (CommNet on, a crew-less manifest: no crewed part)
    short = long_ = False
    for p in parts:
        for m in db["modules"].get(p["name"], []):
            mi = m["module_index"]
            if m["class"] in ("ModuleDataTransmitter", "ModuleGroundCommsPart"):
                t = db["fields"].get((p["name"], mi), {}).get("antennaType", {}).get("value", "DIRECT")
                if t == "INTERNAL":
                    short = True
                else:
                    long_ = True
    res["AntennaPresent"] = {"pass": bool(short and long_)}
    # electric charge
    def has_ec(p):
        return any(r["name"] == "ElectricCharge" for r in db["resources"].get(p["name"], [])) or any(r.get("name") == "ElectricCharge" for r in p["raw"]["resources"])
    def is_producer(p):
        if "ModuleDeployableSolarPanel" in classes(db, p["name"]):
            return True
        if p["name"] != "launchClamp1":
            for m in db["modules"].get(p["name"], []):
                mi = m["module_index"]
                if m["class"] == "ModuleGenerator":
                    # the generator's OUTPUT_RESOURCE nodes are not in the module_fields table: use the cfg-independent rule below
                    return None
        return False
    # (generator outputs are not in the database tables: producers are decided by solar panels only when no generator exists)
    batteries = [p["i"] for p in parts if has_ec(p) and "ModuleCommand" not in classes(db, p["name"]) and "ModuleAlternator" not in classes(db, p["name"])]
    prod = [is_producer(p) for p in parts]
    if None not in prod:
        producers = [p["i"] for p in parts if prod[p["i"]]]
        res["ElectricChargeAndNoBattery"] = {"pass": not (producers and not batteries)}
        res["ElectricBatteryAndNoCharge"] = {"pass": not (batteries and not producers)}
    # MissingCrew: crew-less manifest -> every crewable part is reported
    crewable = [p["i"] for p in parts if db["parts"][p["name"]]["CrewCapacity"] > 0]
    res["MissingCrew"] = {"pass": not crewable, "failed": crewable}
    # NoControlSources (crew-less manifest): a control source exists when a part with isControlSource has no ModuleCommand or minimumCrew <= 0
    ok = False
    for p in parts:
        cls = db["modules"].get(p["name"], [])
        cmd = [m["module_index"] for m in cls if m["class"] == "ModuleCommand"]
        if cmd:
            mc = int(db["fields"][(p["name"], cmd[0])].get("minimumCrew", {}).get("value", "0"))
            if mc <= 0:
                ok = True
    res["NoControlSources"] = {"pass": ok}
    # LadderPresent / LandingGearPresent
    wheel = any("ModuleWheelBase" in classes(db, p["name"]) for p in parts)
    ladder = any("RetractableLadder" in classes(db, p["name"]) or "ladder" in p["name"].lower() for p in parts)
    res["LadderPresent"] = {"pass": (not wheel) or ladder}
    free = False
    for p in parts:
        for m in db["modules"].get(p["name"], []):
            mi = m["module_index"]
            if m["class"] == "ModuleWheelBase":
                wt = db["fields"].get((p["name"], mi), {}).get("wheelType", {}).get("value", "FREE")
                if wt == "FREE":
                    free = True
                    break
                # MOTORIZED continues the loop; LEG is ignored
        if free:
            break
    res["LandingGearPresent"] = {"pass": free}
    # StationHubAttachments: a stationHub with a free attach node (its node count comes from the prefab, the links from the craft)
    hubs = []
    for p in parts:
        if p["name"] == "stationHub":
            linked = set()
            for an in p["raw"].get("attN", []):
                nid, _, rest = an.partition(",")
                if not rest.startswith("Null"):
                    linked.add(nid)
            if any(n["id"] not in linked for n in db["nodes"].get("stationHub", []) if n["id"] != ""):   # the id-less row is the surface node placeholder
                hubs.append(p["i"])
    res["StationHubAttachments"] = {"pass": not hubs, "failed": hubs}
    # NonRootCmdMissaligned
    failed = []
    rr = parts[root]["rot"]
    for p in parts:
        if p["i"] != root and "ModuleCommand" in classes(db, p["name"]):
            a1 = vangle(qmul_vec(p["rot"], UP), qmul_vec(rr, UP))
            a2 = vangle(qmul_vec(p["rot"], FORWARD), qmul_vec(rr, FORWARD))
            if a1 > F(5.0) or a2 > F(5.0):
                failed.append(p["i"])
    res["NonRootCmdMissaligned"] = {"pass": not failed, "failed": failed}
    # ParachuteOnEngineStage (inverseStage = the craft's istg)
    def istg(p):
        return int(p["raw"].get("istg", "0"))
    chutes = [p["i"] for p in parts if "ModuleParachute" in classes(db, p["name"])]
    engines = [p["i"] for p in parts if "ModuleEngines" in classes(db, p["name"]) or "ModuleEnginesFX" in classes(db, p["name"])]
    failed = [c for c in chutes if any(istg(parts[c]) == istg(parts[e]) for e in engines)]
    res["ParachuteOnEngineStage"] = {"pass": not failed, "failed": failed}
    # EnginesJettisonedBeforeUse
    failed = []
    def rec(p, orig):
        if p["parent"] < 0:
            return False
        par = parts[p["parent"]]
        if "ModuleDecouple" in classes(db, par["name"]) and int(par["raw"].get("sqor", "-1")) != -1 or False:
            pass
        return None
    # inStageIndex (sidx) and inverseStage (istg) as the loader assigns them
    for p in parts:
        if "ModuleEngines" in classes(db, p["name"]) or "ModuleEnginesFX" in classes(db, p["name"]):
            q = p
            hit = False
            while q["parent"] >= 0:
                par = parts[q["parent"]]
                if "ModuleDecouple" in classes(db, par["name"]) and int(par["raw"].get("sidx", "-1")) != -1 and istg(par) >= istg(p):
                    hit = True
                    break
                q = par
            if hit:
                failed.append(p["i"])
    res["EnginesJettisonedBeforeUse"] = {"pass": not failed, "failed": failed}
    # RequiredIScienceDataTransmitter
    has_tx = any("ModuleDataTransmitter" in classes(db, p["name"]) for p in parts)
    surveyors = [p["i"] for p in parts if "ModuleOrbitalSurveyor" in classes(db, p["name"])]
    res["RequiredIScienceDataTransmitter"] = {"pass": has_tx or not surveyors, "failed": [] if has_tx else surveyors}
    # DecouplerFacing: ModuleDecouple parts whose explosive node is attached to something other than the parent
    failed = []
    for p in parts:
        cls = db["modules"].get(p["name"], [])
        dec = [m["module_index"] for m in cls if m["class"] == "ModuleDecouple"]
        if not dec:
            continue
        fields = db["fields"].get((p["name"], dec[0]), {})
        node_id = fields.get("explosiveNodeID", {}).get("value", "top")
        omni = fields.get("isOmniDecoupler", {}).get("value", "False") in ("True", True)
        partner = None
        for an in p["raw"].get("attN", []):
            nid, _, rest = an.partition(",")
            if nid == node_id:
                partner = rest.split("_")[0:2]
                partner = None if rest.startswith("Null") else split_id(rest.split("_")[0] + "_" + rest.split("_")[1])[1]
        has_node = any(n["id"] == node_id for n in db["nodes"].get(p["name"], []))
        parent_id = parts[p["parent"]]["id"] if p["parent"] >= 0 else None
        node_attached = partner
        if not omni and has_node and node_attached != parent_id:
            failed.append(p["i"])
    res["DecouplerFacing"] = {"pass": not failed, "failed": failed}
    return res


# ---------------------------------------------------------------------------------------------------------------------------------------
def main():
    db = load_part_db()
    out = {"description": "WP-40 editor reference (tools/wp40/editor_reference.py): numpy float32 / Mono float rule, no kspcore code", "crafts": {}}
    for label, path in craft_files():
        parts = build_craft(label, path, db)
        known = all(p["name"] in db["parts"] for p in parts)
        entry = {"names": [p["name"] for p in parts], "ids": [p["id"] for p in parts], "known": known,
                 "parent": [p["parent"] for p in parts], "pos": [[bits(x) for x in p["pos"]] for p in parts],
                 "rot": [[bits(x) for x in p["rot"]] for p in parts]}
        entry["local_pose"] = sec_local_pose(parts)
        if known:
            entry["symmetry"] = sec_symmetry(parts, db)
            entry["mirror_helpers"] = sec_mirror_helpers(parts)
            entry["gizmos"] = sec_gizmos(parts)
            entry["com"] = sec_com(parts, db)
            entry["cot"] = sec_cot(parts, db)
            entry["preflight"] = sec_preflight(parts, db)
        out["crafts"][label] = entry
    json.dump(out, sys.stdout, separators=(",", ":"))
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
