#!/usr/bin/env python3
"""WP-29e: independent reference of VPWheelCollider.UpdateVisualWheel's Transform writes on the stock wheel models -> core/tests/data/wp29e_visual_reference.json.

The C# (VehiclePhysics/VPWheelCollider.cs:275-300), after m_suspensionPosition, m_visualSteerAngle and m_angularPosition are computed:

    if (suspensionTransform != null && updateSuspension)                       suspensionTransform.position = m_suspensionPosition;
    if (caliperTransform != null && updateCaliper) {
        if (suspensionTransform == null || !m_isCaliperChildOfSuspension)      caliperTransform.position = m_suspensionPosition;
        caliperTransform.localRotation = Quaternion.Euler(0f, steer, 0f);
    }
    if (wheelTransform != null && updateWheel) {
        if ((suspensionTransform == null && caliperTransform == null) || (!m_isWheelChildOfSuspension && !m_isWheelChildOfCaliper))
                                                                               wheelTransform.position = m_suspensionPosition;
        if (caliperTransform != null && m_isWheelChildOfCaliper)               wheelTransform.localRotation = Quaternion.Euler(spin * 57.29578f, 0f, 0f);
        else                                                                   wheelTransform.localRotation = Quaternion.Euler(spin * 57.29578f, steer, 0f);
    }

Re-written here in numpy float32 (one rounding per operation) over the reduced stock models (core/data/wheel_models, parsed with the WP-29d reader, the
Transform hierarchy of PartLoader.CompileModel as in wheel_model_reference.py):

  * Transform.position = p (world) on a node with a parent: localPosition = parent.InverseTransformPoint(p) = (rotate back by the parent's
    world rotation (the conjugate), divide by its accumulated scale) -- the part transform is the world frame here, composed with a random part pose;
  * Transform.localRotation = q;
  * Quaternion.Euler(x, y, z) (degrees): euler * (float)(Math.PI / 180) then the native EulerToQuaternion (qY * qX) * qZ of the half-angle sin / cos --
    the CRT's float sinf / cosf modelled as the double function rounded to float (the C++ uses the UCRT's: within ~2 ULP of this);
  * the Transform.IsChildOf flags either from the hierarchy ("re-enabled") or all false (the values OnEnableVehicle computes while the transform fields are
    still unassigned at KSPWheelController.Create).

Each case: a part (stock wheel / gear / leg model), a rescale factor, a random part pose, the update flags, then three consecutive writes on the same
rig with random m_suspensionPosition (world), steer angle and spin angle. Expected after every write: the part-space pose (position, rotation, scale
accumulated) of the three moved nodes and of every node that carries a collider chunk, pre-order indices of the rig.

Usage:  python -I core/tools/wp29e/visual_reference.py > core/tests/data/wp29e_visual_reference.json   (byte-reproducible, seed 2905)
"""
import hashlib
import json
import math
import os
import random
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "wp29d"))
import mu_wheel_reader as R  # noqa: E402
import wheel_model_reference as W  # noqa: E402

F = np.float32
SEED = 2905
CORE = os.path.join(HERE, "..", "..")
REDUCED = os.path.join(CORE, "data", "wheel_models")
GAMEDATA = os.path.join(CORE, "..", "harness", "inputs")  # GameData/Squad/Parts/... cfg mirror


def conj(q):
    return (F(-q[0]), F(-q[1]), F(-q[2]), q[3])


def euler(deg):
    """Quaternion.Euler(Vector3) in degrees: managed * (float)Math.PI/180f, native EulerToQuaternion = (qY * qX) * qZ of the half angles."""
    d2r = F(0.0174532924)
    e = [F(F(a) * d2r) for a in deg]
    half = [F(x / F(2.0)) for x in e]
    c = [F(math.cos(float(h))) for h in half]
    s = [F(math.sin(float(h))) for h in half]
    qx = (s[0], F(0), F(0), c[0])
    qy = (F(0), s[1], F(0), c[1])
    qz = (F(0), F(0), s[2], c[2])
    return W.qmul(W.qmul(qy, qx), qz)


def inverse_transform_point(pose, p):
    """NativeTransform::InverseTransformPoint: (p - position) rotated by the conjugate (float per operation)."""
    pos, rot = pose[:3], pose[3:7]
    d = [F(F(p[i]) - F(pos[i])) for i in range(3)]
    return W.qrot(conj(rot), d)


def set_world_position(node, part_pose, world):
    """Transform.position = world, with the part transform as the world frame (scale one): the local position under the current parent."""
    p = inverse_transform_point(part_pose, world)
    if node.parent is None:
        node.lp = [F(x) for x in p]
        return
    ps = W.part_space(node.parent)
    d = W.qrot(conj(ps[3:7]), [F(F(p[i]) - F(ps[i])) for i in range(3)])
    node.lp = [F(F(d[i]) / F(ps[7 + i])) for i in range(3)]


def is_child_of(node, anc):
    n = node
    while n is not None:
        if n is anc:
            return True
        n = n.parent
    return False


def write_visual(susp, cal, wheel, upd, child, part_pose, sp, steer, spin):
    """VPWheelCollider.cs:275-300."""
    if susp is not None and upd[0]:
        set_world_position(susp, part_pose, sp)
    if cal is not None and upd[1]:
        if susp is None or not child[0]:
            set_world_position(cal, part_pose, sp)
        cal.lr = list(euler((0.0, steer, 0.0)))
    if wheel is not None and upd[2]:
        if (susp is None and cal is None) or (not child[1] and not child[2]):
            set_world_position(wheel, part_pose, sp)
        s = F(F(spin) * F(57.29578))
        if cal is not None and child[2]:
            wheel.lr = list(euler((float(s), 0.0, 0.0)))
        else:
            wheel.lr = list(euler((float(s), steer, 0.0)))


def f32list(xs):
    return [float(F(x)) for x in xs]


def rand_unit_quat(rng):
    while True:
        q = [rng.uniform(-1, 1) for _ in range(4)]
        n = math.sqrt(sum(x * x for x in q))
        if 0.1 < n <= 1.0:
            return [F(x / n) for x in q]


def main():
    rng = random.Random(SEED)
    manifest = json.load(open(os.path.join(REDUCED, "MANIFEST.json")))["parts"]
    out = {"format": "kspcore-wp29e-visual/1",
           "note": "independent numpy float32 transcription of VPWheelCollider.UpdateVisualWheel's Transform writes (VPWheelCollider.cs:275-300) over the "
                   "reduced stock wheel / gear / leg models (core/data/wheel_models, origins in its MANIFEST.json); tools/wp29e/visual_reference.py, seed %d" % SEED,
           "origin": {}, "cases": []}
    for name in sorted(manifest):
        rec = manifest[name]
        cfg_rel = rec["cfg"]
        cfg_path = os.path.join(GAMEDATA, cfg_rel)
        cfg_bytes = open(cfg_path, "rb").read()
        rescale_cfg, cfg_models, names = W.parse_cfg(cfg_path)
        susp_n, cal_n, wheel_n = names.get("suspensionTransformName"), names.get("caliperTransformName"), names.get("wheelTransformName")
        if not (susp_n or cal_n or wheel_n):
            continue
        roots = []
        models = []
        for inst in rec["instances"]:
            data = open(os.path.join(REDUCED, inst["file"]), "rb").read()
            ver, root = R.read_wheel_mu(data)
            roots.append(root)
            if inst.get("fromModelNode"):
                mm = {"parent": inst.get("parent", "")}
                if inst.get("scale") is not None and "scale" in inst and inst["scale"] != [1.0, 1.0, 1.0]:
                    mm["scale"] = inst["scale"]
                models.append(mm)
        out["origin"][name] = {"cfg": cfg_rel, "cfgSha256": hashlib.sha256(cfg_bytes).hexdigest(),
                               "mu": [{"file": i["file"], "reducedSha256": hashlib.sha256(open(os.path.join(REDUCED, i["file"]), "rb").read()).hexdigest()}
                                      for i in rec["instances"]]}
        for rescale in (rescale_cfg, F(1.5)):
            for variant in range(5):
                nodes = W.build(roots, rescale, models if models else None)
                pre = W.preorder(nodes[0])
                susp = W.find(nodes[0], susp_n) if susp_n else None
                cal = W.find(nodes[0], cal_n) if cal_n else None
                wheel = W.find(nodes[0], wheel_n) if wheel_n else None
                if (susp_n and susp is None) or (cal_n and cal is None) or (wheel_n and wheel is None):
                    raise SystemExit("%s: a named transform is missing" % name)
                moved = [n for n in (susp, cal, wheel) if n is not None]
                carriers = [i for i, n in enumerate(pre) if (n.src is not None and n.src.colliders) or n in moved]
                part_pose = [F(rng.uniform(-5, 5)) for _ in range(3)] + rand_unit_quat(rng)
                upd = [True, True, True] if variant < 3 else [rng.random() < 0.7 for _ in range(3)]
                if variant % 2 == 0:
                    child = [bool(cal and susp and is_child_of(cal, susp)), bool(wheel and susp and is_child_of(wheel, susp)),
                             bool(wheel and cal and is_child_of(wheel, cal))]
                else:
                    child = [False, False, False]
                case = {"part": name, "rescale": float(rescale), "partPose": f32list(part_pose), "update": upd, "childOf": child,
                        "carriers": carriers, "moved": [pre.index(n) if n is not None else -1 for n in (susp, cal, wheel)], "steps": []}
                for step in range(3):
                    base = W.part_space(wheel or cal or susp)
                    local = [F(base[i] + F(rng.uniform(-0.3, 0.3))) for i in range(3)]
                    # a world point: the part pose applied to a part-space point near the wheel
                    sp = W.qrot(tuple(part_pose[3:7]), local)
                    sp = [F(sp[i] + part_pose[i]) for i in range(3)]
                    steer = F(rng.uniform(-35.0, 35.0))
                    spin = F(rng.uniform(0.0, 6.2831853))
                    write_visual(susp, cal, wheel, upd, child, part_pose, sp, float(steer), float(spin))
                    res = [f32list(W.part_space(pre[i])) for i in carriers]
                    loc = [f32list(n.lp + n.lr) if n is not None else None for n in (susp, cal, wheel)]
                    case["steps"].append({"suspensionPosition": f32list(sp), "steer": float(steer), "spin": float(spin), "poses": res, "local": loc})
                out["cases"].append(case)
    json.dump(out, sys.stdout, indent=None, separators=(",", ":"), sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
