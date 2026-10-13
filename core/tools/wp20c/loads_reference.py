#!/usr/bin/env python3
"""WP-20c: independent reference for the structural load model of the reduced rigid-body model -- test data generator.

Re-implements, in numpy (float64) and from the model's definition (core/include/ksp/physics/structure_graph.hpp, core/PHYSICS-SPIKE.md
2.8), with NO code shared with kspcore and different algorithms where kspcore has a fast one:
  * the demand of a rigid part moving with its cluster:  F = m (a + alpha x r + w x (w x r) - g) - F_ext,  M = I alpha + w x (I w) - M_ext;
  * the tree solve as explicit subtree sums (a depth-first set of the parts behind every joint, moments taken directly about the joint's
    anchor centroid; kspcore accumulates children into parents in one pass);
  * the per-joint sub-joint split as a dense 6x6 solve of the joint's elements built from their strain map (s = J u);
  * the indeterminate graph solve as ONE dense global system K = sum J^T k J over every element (kspcore: sparse block Cholesky with a
    minimum-degree order), the fixed node of each piece without ground = its heaviest node (first on a tie), the ground fixed itself.
Element model (PhysX's ConfigurableJoint per PartJoint sub-joint, PartJoint.cs:176-205 / :359-395): force on the b (host) node at the anchor
f = -kl (t_b + th_b x r_b - t_a - th_a x r_a), angular-drive torque on b = -ka (th_b - th_a); a ground support: a = -1 (fixed), anchor at
the node's centre of mass.

The structures are synthetic (seeded numpy generator): random trees of 2-12 rigid parts with 1- or 3-sub-joint joints (rings of 120
degrees around the joint axis, as PartJoint.create builds them), stiffnesses of KSP's range (kl 1e8 quasi-rigid; ka 24,000 / 60,000 /
90,000 = 1500 num3 10 for size-1 surface / size-1 stack / size-2 stack nodes; rigid: kl), struts / autostruts closing loops, ground supports;
masses 0.05-20 t, own tensors random SPD, cluster kinematics and external loads random. The raw demands are written, then balanced
(the root node takes the opposite of their total force and moment, as a vessel's residual makes them self-equilibrated) and the solves
run on the balanced ones. The reference also checks itself: on every tree case its dense graph loads equal its tree split.

Usage: python -I core/tools/wp20c/loads_reference.py > core/tests/data/wp20c_loads_reference.json
"""
import json
import sys

import numpy as np

KL = 1.0e8
KA_CHOICES = [24000.0, 60000.0, 90000.0, KL]


def skew(r):
    return np.array([[0.0, -r[2], r[1]], [r[2], 0.0, -r[0]], [-r[1], r[0], 0.0]])


def demand(m, x, I, com, a, alpha, w, g, F, M):
    r = x - com
    acc = a + np.cross(alpha, r) + np.cross(w, np.cross(w, r)) - g
    return m * acc - F, I @ alpha + np.cross(w, I @ w) - M


def random_tensor(rng, m):
    d = rng.uniform(0.2, 3.0, 3) * max(1.0, m)
    q = rng.normal(size=4)
    q /= np.linalg.norm(q)
    x, y, z, w = q
    R = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                  [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                  [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])
    return R @ np.diag(d) @ R.T


def strain_map(n_nodes, el, com):
    """3 x 6n maps from the node displacements (t, th per node) to the element's linear strain s and its angular strain th_b - th_a"""
    S = np.zeros((3, 6 * n_nodes))
    A = np.zeros((3, 6 * n_nodes))
    p = np.array(el["anchor"])
    b = el["b"]
    rb = p - com[b]
    S[:, 6 * b:6 * b + 3] += np.eye(3)
    S[:, 6 * b + 3:6 * b + 6] += -skew(rb)  # th x r = -r x th
    A[:, 6 * b + 3:6 * b + 6] += np.eye(3)
    if el["a"] >= 0:
        a = el["a"]
        ra = p - com[a]
        S[:, 6 * a:6 * a + 3] -= np.eye(3)
        S[:, 6 * a + 3:6 * a + 6] -= -skew(ra)
        A[:, 6 * a + 3:6 * a + 6] -= np.eye(3)
    return S, A


def graph_solve(n, com, mass, elements, dem):
    K = np.zeros((6 * n, 6 * n))
    maps = []
    for el in elements:
        S, A = strain_map(n, el, com)
        K += el["kl"] * S.T @ S + el["ka"] * A.T @ A
        maps.append((S, A))
    # pieces: union over the elements, the ground (index n) is fixed
    parent = list(range(n + 1))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for el in elements:
        a = el["a"] if el["a"] >= 0 else n
        parent[find(a)] = find(el["b"])
    best = {}
    for k in range(n):
        r = find(k)
        if r not in best or mass[k] > mass[best[r]]:
            best[r] = k
    ground_root = find(n)
    fixed = set(best[r] for r in best if r != ground_root)
    free = [k for k in range(n) if k not in fixed]
    idx = [6 * k + c for k in free for c in range(6)]
    rhs = np.zeros(6 * n)
    for k in range(n):
        rhs[6 * k:6 * k + 3] = -dem[k][0]
        rhs[6 * k + 3:6 * k + 6] = -dem[k][1]
    u = np.zeros(6 * n)
    if idx:
        u[idx] = np.linalg.solve(K[np.ix_(idx, idx)], rhs[idx])
    loads = []
    for el, (S, A) in zip(elements, maps):
        loads.append((-el["kl"] * (S @ u), -el["ka"] * (A @ u)))
    return loads


def local_split(anchors_rel, kls, kas, F, M):
    K = np.zeros((6, 6))
    for r, kl, ka in zip(anchors_rel, kls, kas):
        J = np.hstack([np.eye(3), -skew(r)])  # strain of the loaded side's anchor: t + th x r
        Ja = np.hstack([np.zeros((3, 3)), np.eye(3)])
        K += kl * J.T @ J + ka * Ja.T @ Ja
    u = np.linalg.solve(K, np.concatenate([F, M]))
    t, th = u[:3], u[3:]
    return [(kl * (t + np.cross(th, r)), ka * th) for r, kl, ka in zip(anchors_rel, kls, kas)]


def subtree(children, i):
    out, stack = [], [i]
    while stack:
        k = stack.pop()
        out.append(k)
        stack.extend(children[k])
    return out


def vec(v):
    return [float(x) for x in v]


def make_case(rng, name, n, loops, supports):
    com = [np.zeros(3)]
    parent = [-1]
    for i in range(1, n):
        p = int(rng.integers(0, i))
        d = rng.normal(size=3)
        d /= np.linalg.norm(d)
        com.append(com[p] + d * rng.uniform(1.0, 3.0))
        parent.append(p)
    mass = [float(rng.uniform(0.05, 20.0)) for _ in range(n)]
    tensors = [random_tensor(rng, m) for m in mass]
    elements, joints = [], []
    parent_joint = [-1] * n
    for i in range(1, n):
        p = parent[i]
        axis = com[i] - com[p]
        axis /= np.linalg.norm(axis)
        c = 0.5 * (com[i] + com[p]) + rng.normal(size=3) * 0.1
        subs = 3 if rng.random() < 0.5 else 1
        ka = float(rng.choice(KA_CHOICES))
        j = len(joints)
        parent_joint[i] = j
        members = []
        if subs == 3:
            tmp = np.cross(axis, [1.0, 0.0, 0.0])
            if np.linalg.norm(tmp) < 0.1:
                tmp = np.cross(axis, [0.0, 1.0, 0.0])
            e1 = tmp / np.linalg.norm(tmp)
            e2 = np.cross(axis, e1)
            radius = rng.uniform(0.5, 1.5)
            for s in range(3):
                ang = 2.0 * np.pi * s / 3.0
                members.append(len(elements))
                elements.append({"a": p, "b": i, "anchor": c + radius * (np.cos(ang) * e1 + np.sin(ang) * e2), "kl": KL, "ka": ka, "joint": j, "sub": s})
        else:
            members.append(len(elements))
            elements.append({"a": p, "b": i, "anchor": c, "kl": KL, "ka": ka, "joint": j, "sub": 0})
        anchors = [elements[e]["anchor"] for e in members]
        joints.append({"centroid": np.mean(anchors, axis=0), "elements": members, "tree": True})
    for _ in range(loops):  # struts / autostruts: one sub-joint between two parts that are not parent and child
        while True:
            a, b = (int(x) for x in rng.integers(0, n, 2))
            if a != b and parent[a] != b and parent[b] != a:
                break
        t = rng.uniform(0.2, 0.8)
        p = com[a] * (1 - t) + com[b] * t
        j = len(joints)
        elements.append({"a": a, "b": b, "anchor": p, "kl": KL, "ka": 24000.0, "joint": j, "sub": 0})
        joints.append({"centroid": p, "elements": [len(elements) - 1], "tree": False})
    for k in rng.choice(n, size=supports, replace=False) if supports else []:
        elements.append({"a": -1, "b": int(k), "anchor": com[int(k)].copy(), "kl": KL, "ka": KL, "joint": -1, "sub": 0})
    kin = {"com": rng.normal(size=3), "a": rng.normal(size=3) * 5.0, "alpha": rng.normal(size=3) * 0.5,
           "w": rng.normal(size=3) * 0.5, "g": rng.normal(size=3) * 10.0}
    ext = [(rng.normal(size=3) * 50.0, rng.normal(size=3) * 20.0) for _ in range(n)]
    raw = [demand(mass[k], com[k], tensors[k], kin["com"], kin["a"], kin["alpha"], kin["w"], kin["g"], ext[k][0], ext[k][1]) for k in range(n)]
    # balance: the root takes the opposite of the total (force; moment about the root's centre of mass)
    Ft = sum(d[0] for d in raw)
    Mt = sum(d[1] + np.cross(com[k] - com[0], d[0]) for k, d in enumerate(raw))
    bal = [(d[0].copy(), d[1].copy()) for d in raw]
    bal[0] = (bal[0][0] - Ft, bal[0][1] - Mt)
    case = {"name": name, "nodes": [], "kin": {k: vec(v) for k, v in kin.items()}, "elements": [], "joints": [],
            "parentNode": parent, "parentJoint": parent_joint, "loops": loops, "supports": supports}
    for k in range(n):
        case["nodes"].append({"mass": mass[k], "com": vec(com[k]), "I": [vec(r) for r in tensors[k]], "F": vec(ext[k][0]), "M": vec(ext[k][1]),
                              "demand": {"force": vec(raw[k][0]), "moment": vec(raw[k][1])},
                              "balanced": {"force": vec(bal[k][0]), "moment": vec(bal[k][1])}})
    for el in elements:
        case["elements"].append({"a": el["a"], "b": el["b"], "anchor": vec(el["anchor"]), "kl": el["kl"], "ka": el["ka"], "joint": el["joint"],
                                 "sub": el["sub"]})
    children = [[] for _ in range(n)]
    for i in range(1, n):
        children[parent[i]].append(i)
    split = [None] * len(elements)
    for j, jt in enumerate(joints):
        rec = {"centroid": vec(jt["centroid"]), "elements": jt["elements"], "tree": jt["tree"]}
        if jt["tree"]:
            child = elements[jt["elements"][0]]["b"]
            F = np.zeros(3)
            M = np.zeros(3)
            for k in subtree(children, child):
                F += bal[k][0]
                M += bal[k][1] + np.cross(com[k] - jt["centroid"], bal[k][0])
            rec["resultant"] = {"force": vec(F), "moment": vec(M)}
            parts = local_split([elements[e]["anchor"] - jt["centroid"] for e in jt["elements"]], [elements[e]["kl"] for e in jt["elements"]],
                                [elements[e]["ka"] for e in jt["elements"]], F, M)
            for e, (f, t) in zip(jt["elements"], parts):
                split[e] = (f, t)
        case["joints"].append(rec)
    loads = graph_solve(n, com, mass, elements, bal)
    case["graph"] = [{"force": vec(f), "torque": vec(t)} for f, t in loads]
    case["split"] = [None if s is None else {"force": vec(s[0]), "torque": vec(s[1])} for s in split]
    if loops == 0 and supports == 0:  # self-check: on a tree the dense graph solve is the tree split
        scale = max(np.linalg.norm(f) for f, _ in loads) + 1e-300
        for e, s in enumerate(split):
            assert np.linalg.norm(s[0] - loads[e][0]) < 1e-6 * scale, (name, e)
    return case


def main():
    rng = np.random.default_rng(20261013)
    cases = []
    for k in range(12):
        cases.append(make_case(rng, "tree-%d" % k, int(rng.integers(2, 13)), 0, 0))
    for k in range(8):
        cases.append(make_case(rng, "loops-%d" % k, int(rng.integers(4, 13)), int(rng.integers(1, 4)), 0))
    for k in range(6):
        cases.append(make_case(rng, "supports-%d" % k, int(rng.integers(3, 11)), int(rng.integers(0, 3)), int(rng.integers(1, 4))))
    json.dump({"generator": "core/tools/wp20c/loads_reference.py", "seed": 20261013, "cases": cases}, sys.stdout, indent=0)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
