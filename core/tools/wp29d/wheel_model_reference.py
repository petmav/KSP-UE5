"""WP-29d: independent reference of the wheel / gear / leg model data -> core/tests/data/wp29d_models_reference.json.

Parses the ORIGINAL staged .mu files (core/data/mu_staging_wheels, from stage_wheel_models.py) with mu_wheel_reader (PartReader's grammar,
the WP-17b skippers) and checks that the committed reduced copy (core/data/wheel_models) carries identical content. For each part:

  * every transform in pre-order: name path, local position / rotation / scale (float32 bit patterns), tag, layer, collider chunks, the
    WheelCollider chunk (18 float32 bit patterns);
  * the transform's pose in PART space (PartLoader.cs:1765 CompileModel placement: "model" with localScale rescaleFactor, the MODEL node or
    the directory model; Unity's native Transform maths in float32, one rounding per operation, the quaternion product and the vector
    rotation as in core/src/unity/quaternion_native.cpp / Quaternion.cs);
  * the Animation components: owner, default clip, clips with their curves (path, property, type, wrap modes, key count, FNV-1a 64 of the
    key bytes);
  * SAMPLES: each clip evaluated at fixed fractions of its length with Unity's cached Hermite cubic (AnimationCurveTpl<float>::
    CalculateCacheData, transcribed from harness/src/KspUnityShim/AnimationCurve.cs) in float32, the rotation renormalised; the pose of
    every node afterwards (FNV-1a 64 of the bit patterns) and of the transforms the wheel modules look up (full bit patterns).

Run:  python -I core/tools/wp29d/wheel_model_reference.py [--staging core/data/mu_staging_wheels] [--reduced core/data/wheel_models]
      > core/tests/data/wp29d_models_reference.json
"""
import argparse
import hashlib
import json
import os
import re
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mu_wheel_reader as R  # noqa: E402

F = np.float32
SAMPLE_FRACTIONS = [0.0, 0.25, 0.5, 0.8333333, 1.0, 1.25]
NAMED_KEYS = ("wheelColliderTransformName", "wheelTransformName", "suspensionTransformName", "deployTargetTransformName",
              "retractTransformName", "caliperTransformName", "bogeyTransformName", "suspensionColliderName")


def fbits(x):
    return struct.unpack("<I", struct.pack("<f", float(F(x))))[0]


def hx(x):
    return "%08x" % fbits(x)


def fnv(data):
    h = 0xCBF29CE484222325
    for b in data:
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return "%016x" % h


def qmul(a, b):
    """The native Transform code's Quaternion product (core/src/unity/quaternion_native.cpp Multiply)."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    cx = F(F(ay * bz) - F(az * by))
    cy = F(F(az * bx) - F(ax * bz))
    cz = F(F(ax * by) - F(ay * bx))
    return (F(F(cx + F(aw * bx)) + F(bw * ax)), F(F(cy + F(aw * by)) + F(bw * ay)), F(F(cz + F(aw * bz)) + F(bw * az)),
            F(F(F(aw * bw) - F(ax * bx)) - F(F(ay * by) + F(az * bz))))


def qrot(q, v):
    """Quaternion.operator*(Quaternion, Vector3) (Quaternion.cs), float per operation."""
    x, y, z, w = q
    num, num2, num3 = F(x * F(2)), F(y * F(2)), F(z * F(2))
    num4, num5, num6 = F(x * num), F(y * num2), F(z * num3)
    num7, num8, num9 = F(x * num2), F(x * num3), F(y * num3)
    num10, num11, num12 = F(w * num), F(w * num2), F(w * num3)
    rx = F(F(F(F(F(1) - F(num5 + num6)) * v[0]) + F(F(num7 - num12) * v[1])) + F(F(num8 + num11) * v[2]))
    ry = F(F(F(F(num7 + num12) * v[0]) + F(F(F(1) - F(num4 + num6)) * v[1])) + F(F(num9 - num10) * v[2]))
    rz = F(F(F(F(num8 - num11) * v[0]) + F(F(num9 + num10) * v[1])) + F(F(F(1) - F(num4 + num5)) * v[2]))
    return (rx, ry, rz)


class RNode:
    def __init__(self, name, parent, lp, lr, ls, src):
        self.name, self.parent = name, parent
        self.lp = [F(x) for x in lp]
        self.lr = [F(x) for x in lr]
        self.ls = [F(x) for x in ls]
        self.src = src
        self.children = []
        self.prefab = (list(self.lp), list(self.lr), list(self.ls))


def build(read_roots, rescale, models):
    """CompileModel: a list of (root Tnode) + the cfg MODEL dicts. Returns the flat pre-order list of RNode (index 0 = "model")."""
    nodes = []

    def add(t, parent):
        n = RNode(t.name, parent, t.pos, t.rot, t.scl, t)
        nodes.append(n)
        if parent is not None:
            parent.children.append(n)
        for c in t.children:
            add(c, n)
        return n

    if not models:
        model = add(read_roots[0], None)
        model.name = "model"
        model.lp, model.lr, model.ls = [F(0)] * 3, [F(0), F(0), F(0), F(1)], [F(rescale)] * 3
        model.prefab = (list(model.lp), list(model.lr), list(model.ls))
        return nodes
    model = RNode("model", None, (0, 0, 0), (0, 0, 0, 1), (rescale, rescale, rescale), None)
    nodes.append(model)
    for root, mm in zip(read_roots, models):
        host = model
        if mm.get("parent"):
            cur = model
            for part in mm["parent"].split("/"):
                cur = next((c for c in cur.children if c.name == part), None)
                if cur is None:
                    break
            host = cur or model
        inst = add(root, host)
        inst.lp, inst.lr = [F(0)] * 3, [F(0), F(0), F(0), F(1)]
        inst.ls = [F(x) for x in mm["scale"]] if "scale" in mm else [F(1)] * 3
        if "name" in mm:
            inst.name = mm["name"]
        if "position" in mm:
            inst.lp = [F(x) for x in mm["position"]]
        if "rotation" in mm:
            raise ValueError("MODEL rotation (Quaternion.Euler) is not covered")
        inst.prefab = (list(inst.lp), list(inst.lr), list(inst.ls))
    # nodes of the model parent order: add() appended instance subtrees after the model, in cfg order; the host linkage was set above
    return nodes


def preorder(root):
    out = [root]
    for c in root.children:
        out += preorder(c)
    return out


def part_space(n):
    chain = []
    while n is not None:
        chain.append(n)
        n = n.parent
    pos, rot, scl = [F(0)] * 3, [F(0), F(0), F(0), F(1)], [F(1)] * 3
    for c in reversed(chain):
        scaled = [F(scl[i] * c.lp[i]) for i in range(3)]
        r = qrot(rot, scaled)
        pos = [F(pos[i] + r[i]) for i in range(3)]
        rot = list(qmul(rot, c.lr))
        scl = [F(scl[i] * c.ls[i]) for i in range(3)]
    return pos + rot + scl


def path_of(n):
    p = []
    while n is not None:
        p.append(n.name)
        n = n.parent
    return "/".join(reversed(p))


def find(root, name):
    if root.name == name:
        return root
    for c in root.children:
        r = find(c, name)
        if r is not None:
            return r
    return None


def find_path(frm, path):
    cur = frm
    for part in path.split("/"):
        cur = next((c for c in cur.children if c.name == part), None)
        if cur is None:
            return None
    return cur


# ---- Unity AnimationCurveTpl<float>::Evaluate, clamp both ends (harness/src/KspUnityShim/AnimationCurve.cs) ----
def evaluate(keys, t):
    t = F(t)
    n = len(keys)
    if n == 1:
        return F(keys[0][1])
    if t >= F(keys[-1][0]):
        return F(keys[-1][1])
    if F(keys[0][0]) > t:
        return F(keys[0][1])
    lo, ln = 0, n  # upper_bound on time
    while ln > 0:
        half = ln >> 1
        mid = lo + half
        if F(keys[mid][0]) > t:
            ln = half
        else:
            lo = mid + 1
            ln = ln - half - 1
    lhs, rhs = lo - 1, lo
    L, Rk = keys[lhs], keys[rhs]
    dx = F(F(Rk[0]) - F(L[0]))
    dy = F(F(Rk[1]) - F(L[1]))
    length = F(1e-4) if F(1e-4) > dx else dx
    m1 = F(L[3])
    d1 = F(length * F(Rk[2]))
    d0 = F(length * m1)
    inv = F(F(1) / F(length * length))
    a = F(d1 + d0)
    a = F(a - dy)
    a = F(a - dy)
    a = F(a * inv)
    a = F(a / length)
    b = F(dy + dy)
    b = F(b + dy)
    b = F(b - d0)
    b = F(b - d0)
    b = F(b - d1)
    b = F(b * inv)
    cC, cD = m1, F(L[1])
    if np.isposinf(F(L[3])) or np.isposinf(F(Rk[2])):
        a, b, cC = F(0), F(0), F(0)
    u = F(t - F(L[0]))
    r = F(u * a)
    r = F(r + b)
    r = F(r * u)
    r = F(r + cC)
    r = F(r * u)
    return F(r + cD)


PROPS = ["m_LocalPosition.x", "m_LocalPosition.y", "m_LocalPosition.z", "m_LocalRotation.x", "m_LocalRotation.y", "m_LocalRotation.z",
         "m_LocalRotation.w", "m_LocalScale.x", "m_LocalScale.y", "m_LocalScale.z"]


def sample(nodes, owner, clip, t):
    for n in nodes:
        n.lp, n.lr, n.ls = list(n.prefab[0]), list(n.prefab[1]), list(n.prefab[2])
    touched = set()
    for cu in clip["curves"]:
        if cu["type"] != 0 or cu["prop"] not in PROPS:
            continue
        tgt = owner if cu["path"] == "" else find_path(owner, cu["path"])
        if tgt is None:
            continue
        v = evaluate(cu["keys"], t)
        i = PROPS.index(cu["prop"])
        if i < 3:
            tgt.lp[i] = v
        elif i < 7:
            tgt.lr[i - 3] = v
            touched.add(id(tgt))
        else:
            tgt.ls[i - 7] = v
    for n in nodes:
        if id(n) in touched:
            q = n.lr
            m = F(np.sqrt(F(F(F(q[0] * q[0]) + F(q[1] * q[1])) + F(q[2] * q[2])) + F(q[3] * q[3])))
            if m > F(1e-6):
                n.lr = [F(x / m) for x in q]


def parse_cfg(path):
    text = open(path, encoding="utf-8-sig").read()
    text = "\n".join(l.split("//")[0] for l in text.splitlines())
    m = re.search(r"(?m)^\s*rescaleFactor\s*=\s*(\S+)", text)
    rescale = F(m.group(1)) if m else F(1.25)
    models = [dict(re.findall(r"(?m)^\s*(\w+)\s*=\s*(.+?)\s*$", blk)) for blk in re.findall(r"(?ms)^\s*MODEL\s*\{(.*?)\}", text)]
    for mm in models:
        for k in ("scale", "position"):
            if k in mm:
                mm[k] = [float(x) for x in mm[k].replace(" ", "").split(",")]
    names = {}
    for k in NAMED_KEYS:
        mk = re.search(r"(?m)^\s*%s\s*=\s*(\S+)" % k, text)
        if mk:
            names[k] = mk.group(1)
    return rescale, models, names


def tjson(t):
    return {"tag": t.tag, "layer": t.layer, "colliders": [{"kind": c["kind"], "trigger": bool(c["trigger"])} for c in t.colliders],
            "wheel": None if t.wheel is None else [hx(t.wheel[k]) for k in ("mass", "radius", "suspensionDistance")] + [hx(x) for x in t.wheel["center"]]
            + [hx(t.wheel[k]) for k in ("spring", "damper", "targetPosition")] + [hx(x) for x in t.wheel["forwardFriction"]]
            + [hx(x) for x in t.wheel["sidewaysFriction"]]}


def anim_json(a):
    clips = []
    for cl in a["clips"]:
        ln = F(0)
        curves = []
        for cu in cl["curves"]:
            raw = b"".join(struct.pack("<ffff", *k) for k in cu["keys"])
            for k in cu["keys"]:
                ln = max(ln, F(k[0]))
            curves.append({"path": cu["path"], "prop": cu["prop"], "type": cu["type"], "pre": cu["pre"], "post": cu["post"],
                           "n": len(cu["keys"]), "fnv": fnv(raw)})
        clips.append({"name": cl["name"], "wrap": cl["wrap"], "length": hx(ln), "curves": curves})
    return {"default": a["default"], "auto": bool(a["playAutomatically"]), "clips": clips}


def model_json(mu_bytes, rescale, models, names, mu_files_for_roots):
    ver, root = R.read_wheel_mu(mu_bytes)
    return ver, root


def main():
    ap = argparse.ArgumentParser()
    here = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument("--staging", default=os.path.join(here, "..", "..", "data", "mu_staging_wheels"))
    ap.add_argument("--reduced", default=os.path.join(here, "..", "..", "data", "wheel_models"))
    args = ap.parse_args()
    manifest = json.load(open(os.path.join(args.staging, "MANIFEST.json")))
    redman = json.load(open(os.path.join(args.reduced, "MANIFEST.json")))["parts"]
    out = {"format": "kspcore-wp29d-models/1",
           "note": "independent python parse of the original wheel / gear / leg .mu models (decompiled/KSP-dev, origins and sha256 per part); "
                   "float32 bit patterns as hex; part-space poses and animation samples by tools/wp29d/wheel_model_reference.py",
           "sampleFractions": [hx(f) for f in SAMPLE_FRACTIONS], "parts": {}}
    for name in sorted(manifest):
        files = manifest[name]["files"]
        cfgrec = files[0]
        mus = [f for f in files if f["file"].endswith(".mu")]
        rescale, models, names = parse_cfg(os.path.join(args.staging, name, cfgrec["file"]))
        roots, rroots = [], []
        for rec, rmeta in zip(mus, redman[name]["models"]):
            ver, root = R.read_wheel_mu(open(os.path.join(args.staging, name, rec["file"]), "rb").read())
            rbytes = open(os.path.join(args.reduced, rmeta["file"]), "rb").read()
            if hashlib.sha256(rbytes).hexdigest() != rmeta["sha256"]:
                raise SystemExit("reduced file %s differs from its manifest" % rmeta["file"])
            rver, rroot = R.read_wheel_mu(rbytes)
            roots.append(root)
            rroots.append(rroot)
        nodes = build(roots, rescale, models)
        rnodes = build(rroots, rescale, models)
        entry = {"cfg": cfgrec["origin"], "cfgSha256": cfgrec["sha256"],
                 "mu": [{"file": rm["file"], "origin": rm["origin"], "originalSha256": rm["originalSha256"], "reducedSha256": rm["sha256"]}
                        for rm in redman[name]["models"]],
                 "version": ver, "rescale": hx(rescale), "models": models, "names": names, "nodes": [], "anims": [], "samples": []}
        order = nodes
        # pre-order over the tree starting at "model"
        pre = preorder(nodes[0])
        rpre = preorder(rnodes[0])
        assert len(pre) == len(rpre)
        for n, rn in zip(pre, rpre):
            j = {"path": path_of(n), "pos": [hx(x) for x in n.lp], "rot": [hx(x) for x in n.lr], "scl": [hx(x) for x in n.ls]}
            if n.src is not None:
                j.update(tjson(n.src))
                rj = tjson(rn.src)
                if {k: j[k] for k in rj} != rj:
                    raise SystemExit("reduced model differs in %s %s" % (name, j["path"]))
            j["ps"] = [hx(x) for x in part_space(n)]
            entry["nodes"].append(j)
        owners = []
        for n in pre:
            if n.src is None:
                continue
            for a in n.src.anims:
                owners.append((n, a))
        for (n, a), (rn, ra) in zip(owners, [(m, a) for m in rpre if m.src is not None for a in m.src.anims]):
            aj = anim_json(a)
            if aj != anim_json(ra):
                raise SystemExit("reduced animation differs in %s" % name)
            aj["owner"] = path_of(n)
            entry["anims"].append(aj)
        for ai, (n, a) in enumerate(owners):
            for ci, cl in enumerate(a["clips"]):
                length = max(F(k[0]) for cu in cl["curves"] for k in cu["keys"])
                for fr in SAMPLE_FRACTIONS:
                    t = F(F(fr) * length)
                    sample(pre, n, cl, t)
                    ps = [part_space(m) for m in pre]
                    allh = fnv(b"".join(struct.pack("<I", fbits(x)) for p in ps for x in p))
                    named = {}
                    for key, nm in names.items():
                        tgt = find(nodes[0], nm)
                        if tgt is not None:
                            named[nm] = [hx(x) for x in part_space(tgt)]
                    entry["samples"].append({"anim": ai, "clip": ci, "t": hx(t), "all": allh, "named": named})
        for m in pre:
            m.lp, m.lr, m.ls = list(m.prefab[0]), list(m.prefab[1]), list(m.prefab[2])
        out["parts"][name] = entry
    json.dump(out, sys.stdout, indent=None, separators=(",", ":"), sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
