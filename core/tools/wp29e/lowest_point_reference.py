#!/usr/bin/env python3
"""WP-29e: independent reference of the wheel parts' share of Vessel.getLowestPoint (Vessel.cs:2589-2637) -> core/tests/data/wp29e_lowest_point_reference.json.

For a wheel / gear / leg part of the 13 stock models, at a random world pose and in a given gear state, the list of what getLowestPoint's scan reads of that part:

  for every collider of `parts[i].GetComponentsInChildren<Collider>()` (the ACTIVE GameObjects below the part), non-trigger, on a layer other than 21 / 16 / 20:
      z = collider.bounds.min.z                      for the layers 26 / 27 / 30 (the lowest corner of the world AABB)
      z = collider.ClosestPoint(far point below).z   otherwise (the support point in -z of a convex shape: no stock wheel collider is on another layer)
  and the correction of the lowest one (:2616-2637): name.Contains("WheelCollider") -> minus the radius, minus 0.15; name.Contains("collisionEnhancer") -> minus 0.15.
Here: z, the role by the node name (ordinal, case-sensitive), and the radius of a WheelCollider component (the .mu chunk's: the wheel is not set up).

Everything is recomputed in float64 from the REDUCED stock models (core/data/wheel_models, parsed with the WP-29d reader, the Transform hierarchy of
PartLoader.CompileModel and the animation sampling of wheel_model_reference.py, which are bit-gated there), the cfg values read from the GameData mirror
(harness/inputs/GameData) and Unity's collider geometry: BoxCollider size x lossyScale, SphereCollider radius x max |lossyScale|, CapsuleCollider radius x max of
the two perpendicular scales and height x the axis scale, MeshCollider vertices x lossyScale (convex hull of the collider mesh), the WheelCollider chunk's
centre. The support points are the exact oriented-box / sphere / capsule / vertex extremes (the C++ computes corners and endpoints in float32).

State: a gear part is posed at its ModuleWheelDeployment state as ModuleWheelDeployment.OnStart does (stateString "" or "Deployed": normalizedTime =
deployedPosition; "Retracted": 1 - deployedPosition; otherwise `position`); ModuleWheelDamage.SetDamaged switches exactly one of the damaged / undamaged
transforms on (its subtree's colliders are not enumerated when inactive); `hidden` is the wheel transform inactive (VPWheelCollider.hideWheelOnDisable).

Usage:  python -I core/tools/wp29e/lowest_point_reference.py > core/tests/data/wp29e_lowest_point_reference.json   (byte-reproducible, seed 2906)
"""
import hashlib
import json
import math
import os
import random
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "wp29d"))
sys.path.insert(0, os.path.join(HERE, "..", "wp17"))
import mu_transforms as w17  # noqa: E402
import mu_wheel_reader as R  # noqa: E402
import wheel_model_reference as W  # noqa: E402

F = np.float32
SEED = 2906
CORE = os.path.join(HERE, "..", "..")
REDUCED = os.path.join(CORE, "data", "wheel_models")
GAMEDATA = os.path.join(CORE, "..", "harness", "inputs")


class MeshMu(R.WheelMu):
    """The WP-29d reader with the collider meshes' vertices kept (PartReader.cs:495 ReadMesh)."""

    def read_mesh_summary(self):
        r = self.r
        if r.i32() != w17.MeshStart:
            raise ValueError("Mesh Error")
        n = r.i32()
        r.i32()
        verts = []
        while True:
            et = r.i32()
            if et == w17.MeshEnd:
                break
            if et == w17.MeshVertexColors:
                r.take(4 * n)
            elif et == w17.MeshVerts:
                verts = [[r.f32(), r.f32(), r.f32()] for _ in range(n)]
            elif et == w17.MeshNormals:
                r.skip_floats(3 * n)
            elif et in (w17.MeshUV, w17.MeshUV2):
                r.skip_floats(2 * n)
            elif et == w17.MeshTangents:
                r.skip_floats(4 * n)
            elif et == w17.MeshTriangles:
                r.take(4 * r.i32())
            elif et == w17.MeshBoneWeights:
                r.take(32 * n)
            elif et == w17.MeshBindPoses:
                r.skip_floats(16 * r.i32())
        return {"verts": verts}


def read_mu(data):
    mf = MeshMu(data)
    r = mf.r
    if r.i32() != w17.FILE_TYPE_MODEL_BINARY:
        raise ValueError("incorrect file type")
    mf.version = r.i32()
    r.string()
    return mf.read_child(None)


def quat_matrix(q):
    x, y, z, w = [float(v) for v in q]
    n = math.sqrt(x * x + y * y + z * z + w * w)
    x, y, z, w = x / n, y / n, z / n, w / n
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def cfg_value(path, key, default=""):
    text = open(path, encoding="utf-8-sig").read()
    text = "\n".join(l.split("//")[0] for l in text.splitlines())
    m = re.search(r"(?m)^\s*%s\s*=\s*(\S+)" % key, text)
    return m.group(1) if m else default


def lowest_points(nodes, part_pose, hidden, damaged, wheel_n, dmg_n, undmg_n):
    """The (z, role, radius) of every collider getLowestPoint's scan reads of one part, the part at `part_pose` (world position, rotation)."""
    rp = quat_matrix(part_pose[3:7])
    tp = np.array([float(v) for v in part_pose[:3]])
    pre = W.preorder(nodes[0])
    inactive = []
    if dmg_n or undmg_n:
        off = undmg_n if damaged else dmg_n  # SetDamaged(true) switches the undamaged transform off, SetDamaged(false) the damaged one
        if off:
            tgt = W.find(nodes[0], off)
            if tgt is not None:
                inactive.append(W.path_of(tgt))
    if hidden and wheel_n:
        tgt = W.find(nodes[0], wheel_n)
        if tgt is not None:
            inactive.append(W.path_of(tgt))

    def is_inactive(path):
        return any(path == s or path.startswith(s + "/") for s in inactive)

    def role(name):
        if "WheelCollider" in name:
            return "wheel"
        if "collisionEnhancer" in name:
            return "enhancer"
        return "plain"

    out = []
    wheels = []
    for n in pre:
        if n.src is None:
            continue
        ps = W.part_space(n)
        pos = np.array([float(v) for v in ps[:3]])
        rn = quat_matrix(ps[3:7])
        scl = np.array([float(v) for v in ps[7:10]])
        path = W.path_of(n)
        layer = n.src.layer
        for c in n.src.colliders:
            if c["trigger"] or layer in (21, 16, 20) or is_inactive(path):
                continue
            ctr = pos + rn @ (scl * np.array([float(v) for v in c.get("center", [0, 0, 0])]))
            m = rp @ rn  # node axes in the world
            zc = float((rp @ ctr + tp)[2])
            if c["kind"] == "box":
                half = np.abs(scl * np.array([float(v) for v in c["size"]])) / 2.0
                z = zc - float(np.sum(np.abs(m[2, :]) * half))
            elif c["kind"] == "sphere":
                z = zc - float(c["radius"]) * float(np.max(np.abs(scl)))
            elif c["kind"] == "capsule":
                d = c["direction"] if c["direction"] in (0, 1, 2) else 1
                other = max(abs(scl[i]) for i in range(3) if i != d)
                radius = float(c["radius"]) * other
                height = float(c["height"]) * abs(scl[d])
                half_cyl = max(0.0, height / 2.0 - radius)
                z = zc - half_cyl * abs(float(m[2, d])) - radius
            elif c["kind"] == "mesh":
                v = np.array(c["mesh"]["verts"], dtype=np.float64)
                z = float(np.min((rp @ (pos + (rn @ (v * scl).T).T).T).T[:, 2] + tp[2]))
            else:
                continue
            out.append({"z": z, "role": role(n.name), "radius": 0.0})
        if n.src.wheel is not None and not is_inactive(path):
            ctr = pos + rn @ (scl * np.array([float(v) for v in n.src.wheel["center"]]))
            wheels.append({"z": float((rp @ ctr + tp)[2]), "role": role(n.name), "radius": float(n.src.wheel["radius"])})
    return out + wheels


def main():
    rng = random.Random(SEED)
    manifest = json.load(open(os.path.join(REDUCED, "MANIFEST.json")))["parts"]
    out = {"format": "kspcore-wp29e-lowest-point/1",
           "note": "float64 reference of the wheel parts' share of Vessel.getLowestPoint (Vessel.cs:2589-2637) over the reduced stock models; "
                   "tools/wp29e/lowest_point_reference.py, seed %d" % SEED, "origin": {}, "cases": []}
    for name in sorted(manifest):
        rec = manifest[name]
        cfg_path = os.path.join(GAMEDATA, rec["cfg"])
        rescale, cfg_models, names = W.parse_cfg(cfg_path)
        wheel_n = names.get("wheelTransformName")
        dmg_n = cfg_value(cfg_path, "damagedTransformName") or None
        undmg_n = cfg_value(cfg_path, "undamagedTransformName") or None
        anim_trf = cfg_value(cfg_path, "animationTrfName")
        anim_clip = cfg_value(cfg_path, "animationStateName")
        deployed = float(cfg_value(cfg_path, "deployedPosition", "0"))
        roots = []
        models = []
        for inst in rec["instances"]:
            roots.append(read_mu(open(os.path.join(REDUCED, inst["file"]), "rb").read()))
            if inst.get("fromModelNode"):
                mm = {"parent": inst.get("parent", "")}
                if inst.get("scale") is not None and inst["scale"] != [1.0, 1.0, 1.0]:
                    mm["scale"] = inst["scale"]
                models.append(mm)
        out["origin"][name] = {"cfg": rec["cfg"], "cfgSha256": hashlib.sha256(open(cfg_path, "rb").read()).hexdigest(),
                               "mu": [{"file": i["file"], "reducedSha256": hashlib.sha256(open(os.path.join(REDUCED, i["file"]), "rb").read()).hexdigest()}
                                      for i in rec["instances"]]}
        states = [("", 0.0)]
        if anim_trf and anim_clip:
            states = [("", 0.0), ("Deployed", 1.0), ("Retracted", 0.0), ("Deploying...", 0.37), ("Retracting...", 0.81)]
        for k in range(9):
            nodes = W.build(roots, rescale, models if models else None)
            pre = W.preorder(nodes[0])
            st, pos_field = states[k % len(states)]
            if anim_trf and anim_clip:
                owner = next((n for n in pre if n.name == anim_trf and n.src is not None and any(cl["name"] == anim_clip for a in n.src.anims for cl in a["clips"])), None)
                if owner is None:
                    raise SystemExit("%s: animator %s / %s missing" % (name, anim_trf, anim_clip))
                a = next(a for a in owner.src.anims if any(cl["name"] == anim_clip for cl in a["clips"]))
                clip = next(cl for cl in a["clips"] if cl["name"] == anim_clip)
                length = max(F(kk[0]) for cu in clip["curves"] for kk in cu["keys"])
                if st == "Retracted":
                    nrm = 1.0 - deployed
                elif st in ("", "Deployed"):
                    nrm = deployed
                else:
                    nrm = pos_field
                W.sample(pre, owner, clip, F(F(nrm) * length))
            damaged = bool(dmg_n or undmg_n) and (k % 3 == 2)
            hidden = bool(wheel_n) and (k % 4 == 3)
            part_pose = [rng.uniform(-8, 8) for _ in range(3)]
            while True:
                q = [rng.uniform(-1, 1) for _ in range(4)]
                nq = math.sqrt(sum(x * x for x in q))
                if 0.1 < nq <= 1.0:
                    break
            part_pose += [x / nq for x in q]
            part_pose = [float(F(x)) for x in part_pose]
            exp = lowest_points(nodes, part_pose, hidden, damaged, wheel_n, dmg_n, undmg_n)
            out["cases"].append({"part": name, "rescale": float(rescale), "partPose": part_pose,
                                 "stateString": st, "position": pos_field, "deployedPosition": deployed, "damaged": damaged, "hidden": hidden, "expect": exp})
    json.dump(out, sys.stdout, indent=None, separators=(",", ":"), sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
