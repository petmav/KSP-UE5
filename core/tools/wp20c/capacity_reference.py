#!/usr/bin/env python3
"""WP-20c: independent reference for KSP's PartJoint capacity table of the PhysProf crafts -- test data generator.

For every attach joint of the six PhysProf crafts (analysis/physics/craft/*.craft) this computes, from the decompiled C# and with NO code
shared with kspcore, what PartJoint.Create / create / SetupJoint / SetUnbreakable give the joint (Assembly-CSharp/PartJoint.cs):
  * the node (Create, PartJoint.cs:96-137): the child's node to the parent and the parent's node to the child (Part.FindAttachNodeByPart,
    Part.cs:6132: the stack nodes, then the surface node), the smaller of the two (<= picks the child's), or the parent's srfAttachNode;
    rigid = the chosen node's rigid flag (both nodes' when two are found);
  * the sub-joint count (create, :176-205): 3 for a node of size >= 2 when neither part has a ModuleDockingNode, else 1;
  * num3 / stiffness / breakingForce / breakingTorque (SetupJoint, :290-308): num3 = (size + 1) * 2 (stack) or * 0.8f (surface),
    stiffness = 1500 * num3 * FlightGlobals.Stack/SrfAttachStiffNess (10, the flight scene's serialized value [runtime]),
    breaking = Min(host, target).breaking * 1 * num3 / internalJoints;
  * the PhysX limits in flight (SetUnbreakable(false, rigidAttachment), :340-341): breakForce = breakingForce * JointBreakForceFactor *
    (rigid || rigidAttachment ? RigidJointBreakForceFactor : 1), the same for the torque; also with rigidAttachment forced on.
Float rule: the C# float expressions are evaluated in double and rounded to float32 where the C# stores them (the Mono float rule of
kspcore's ports; numpy.float32 conversions here).

Inputs (original game data, never hand-authored):
  analysis/physics/craft/PhysProf-*.craft      part tree (link / attN / srfN), attm, rigidAttachment, persistentId
  harness/captures/out2/part_database.json      the IN-GAME part database (C9 capture): breakingForce / breakingTorque, physicalSignificance,
                                                every attach node's id / size / rigid, the modules of each part
  harness/inputs/Physics.cfg                    jointBreakForceFactor, jointBreakTorqueFactor, rigidJointBreak*Factor
(kspcore reads the parts from the GameData mirror cfgs instead: the capture is an independent source of the same values.)

Usage: python -I core/tools/wp20c/capacity_reference.py > core/tests/data/wp20c_capacity_reference.json
"""
import json
import os
import re
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
CRAFTS = ["PhysProf-S", "PhysProf-S-AS", "PhysProf-M2", "PhysProf-M2-AS", "PhysProf-L2", "PhysProf-L2-AS"]


def f32(x):
    return np.float32(x)


def hexf(x):
    return "%08X" % struct.unpack("<I", struct.pack("<f", float(np.float32(x))))[0]


def load_capture():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "part_database.json")))
    tables = {t["name"]: t for t in d["tables"]}

    def rows(name):
        cols = [c["name"] for c in tables[name]["columns"]]
        return [dict(zip(cols, r)) for r in tables[name]["rows"]]

    parts = {r["name"]: r for r in rows("parts")}
    nodes = {}
    for r in rows("attach_nodes"):
        nodes.setdefault(r["part"], []).append(r)
    docking = set(r["part"] for r in rows("modules") if r["class"] == "ModuleDockingNode")
    return parts, nodes, docking


def physics_cfg():
    out = {}
    for line in open(os.path.join(ROOT, "harness", "inputs", "Physics.cfg"), encoding="utf-8-sig"):
        m = re.match(r"\s*(\w+)\s*=\s*([^/]+)", line)
        if m:
            out[m.group(1)] = m.group(2).strip()
    return {k: np.float32(float(out[k])) for k in ("jointBreakForceFactor", "jointBreakTorqueFactor", "rigidJointBreakForceFactor",
                                                   "rigidJointBreakTorqueFactor")}


def parse_craft(path):
    """the PART nodes in file order: name (part id), pid, links, attN (node id, target ref), srfN target, attm, rigidAttachment"""
    parts, cur, depth = [], None, 0
    for raw in open(path, encoding="utf-8-sig"):
        line = raw.strip()
        if line == "PART" and depth == 0:
            cur = {"links": [], "attN": [], "srfN": None}
            continue
        if line == "{":
            depth += 1
            continue
        if line == "}":
            depth -= 1
            if depth == 0 and cur is not None:
                parts.append(cur)
                cur = None
            continue
        if cur is None or depth != 1 or "=" not in line:
            continue
        k, v = (s.strip() for s in line.split("=", 1))
        if k == "part":
            cur["ref"] = v
            cur["name"] = v.rsplit("_", 1)[0]
        elif k == "persistentId":
            cur["pid"] = int(v)
        elif k == "link":
            cur["links"].append(v)
        elif k == "attN":
            nid, target = v.split(",", 1)
            cur["attN"].append((nid, target.split("|")[0].rsplit("_", 1)[0]))  # "Rockomax32.BW_4294000014_0" -> its part ref
        elif k == "srfN":
            nid, target = v.split(",", 1)
            cur["srfN"] = (nid, target.strip())
        elif k == "attm":
            cur["attm"] = int(v)
        elif k == "rigidAttachment":
            cur["rigid"] = v.lower() == "true"
    return parts


def find_node(part, other_ref, nodes):
    """Part.FindAttachNodeByPart(other): the stack nodes (attachNodes) first, then the surface node"""
    for nid, target in part["attN"]:
        if target == other_ref:
            for n in nodes[part["name"]]:
                if n["idx"] >= 0 and n["id"] == nid:
                    return n
    if part["srfN"] is not None and part["srfN"][1] == other_ref:
        for n in nodes[part["name"]]:
            if n["idx"] == -1:
                return n
    return None


def joint_record(child, parent, db, nodes, docking, pcfg):
    to_parent = find_node(child, parent["ref"], nodes)
    from_parent = find_node(parent, child["ref"], nodes)
    node, rigid = to_parent, False
    if to_parent is None:
        node = from_parent
        if node is not None:
            rigid = bool(node["rigid"])
    elif from_parent is None:
        rigid = bool(node["rigid"])
    else:
        node = to_parent if to_parent["size"] <= from_parent["size"] else from_parent
        rigid = bool(to_parent["rigid"]) or bool(from_parent["rigid"])
    if node is None:
        node = [n for n in nodes[parent["name"]] if n["idx"] == -1][0]
        rigid = bool(node["rigid"])
    size = int(node["size"])
    mode = child.get("attm", 0)
    joints = 3 if size >= 2 and child["name"] not in docking and parent["name"] not in docking else 1
    if mode == 0:
        num3 = f32((float(size) + 1.0) * 2.0)
    else:
        num3 = f32((float(size) + 1.0) * float(np.float32(0.8)))
    stiffness = f32(1500.0 * float(num3) * 10.0)
    hp, tp = db[child["name"]], db[parent["name"]]  # host = child, target = parent: both FULL (PartJoint.cs:145)
    bf_min = min(np.float32(hp["breakingForce"]), np.float32(tp["breakingForce"]))
    bt_min = min(np.float32(hp["breakingTorque"]), np.float32(tp["breakingTorque"]))
    breaking_force = f32(float(bf_min) * 1.0 * float(num3) / float(joints))
    breaking_torque = f32(float(bt_min) * 1.0 * float(num3) / float(joints))

    def limits(flag):
        return (f32(float(breaking_force) * float(pcfg["jointBreakForceFactor"]) * float(pcfg["rigidJointBreakForceFactor"] if flag else np.float32(1.0))),
                f32(float(breaking_torque) * float(pcfg["jointBreakTorqueFactor"]) * float(pcfg["rigidJointBreakTorqueFactor"] if flag else np.float32(1.0))))

    bf, bt = limits(rigid or child.get("rigid", False))
    bfr, btr = limits(True)
    return {"child": child["pid"], "parent": parent["pid"], "childName": child["name"], "size": size, "mode": mode, "internalJoints": joints,
            "rigid": rigid, "stiffness": hexf(stiffness), "breakingForce": hexf(breaking_force), "breakingTorque": hexf(breaking_torque),
            "breakForce": hexf(bf), "breakTorque": hexf(bt), "breakForceRigid": hexf(bfr), "breakTorqueRigid": hexf(btr),
            "breakForceValue": float(bf), "breakTorqueValue": float(bt)}


def main():
    db, nodes, docking = load_capture()
    pcfg = physics_cfg()
    out = {"generator": "core/tools/wp20c/capacity_reference.py", "physics": {k: float(v) for k, v in pcfg.items()}, "crafts": {}}
    for craft in CRAFTS:
        parts = parse_craft(os.path.join(ROOT, "analysis", "physics", "craft", craft + ".craft"))
        by_ref = {p["ref"]: p for p in parts}
        parent_of = {}
        for p in parts:
            for link in p["links"]:
                parent_of[link] = p
        recs = []
        for p in parts:
            parent = parent_of.get(p["ref"])
            if parent is None or db[p["name"]]["physicalSignificance"] != "FULL" or db[parent["name"]]["physicalSignificance"] != "FULL":
                continue
            recs.append(joint_record(p, parent, db, nodes, docking, pcfg))
        out["crafts"][craft] = recs
        assert len(by_ref) == len(parts)
    json.dump(out, sys.stdout, indent=1)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
