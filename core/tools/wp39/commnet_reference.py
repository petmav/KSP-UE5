#!/usr/bin/env python3
"""WP-39: independent reference for CommNet (range model, link creation, horizon occlusion, best-path search, antenna aggregation, control
state) and the data-side maths of ModuleDataTransmitter / ModuleKerbNetAccess -- test data generator.

Re-implements, in plain Python and from the decompiled C# (it shares NO code with kspcore):
  CommRangeModel.cs, CommNetwork.cs (SetNodeConnection, TryConnect, TestOcclusion, FindPath, FindClosestWhere, UpdateShortestPath/Where),
  CommLink.cs (GetSignalStrength), CommPath.UpdateFromPath, NodeUtilities.ConvertSignalStrength, OccluderHorizonCulling.cs, DoubleCurve.Evaluate,
  CommNetBody.CreateOccluder, GameVariables.GetDSNRange, the .NET Dictionary<K,V> enumeration order (entries array + LIFO free list: Node derives
  from Dictionary), CommNetVessel.UpdateComm (the antenna aggregation), CommNetVessel.UpdateControlState / GetControlLevel,
  ModuleDataTransmitter's packet bookkeeping (transmitQueuedData), ModuleKerbNetAccess's FoV / situation rules.
Managed doubles are Python floats; managed floats are numpy.float32 stores (Mono float rule: float expressions evaluate in double and are rounded
where the C# stores them, st() below).

Inputs (original game data, never hand-authored):
  testdata/GameData-mirror/GameData/Squad/Parts/**/*.cfg    every ModuleDataTransmitter (antennaType, antennaPower, antennaCombinable,
                                                            antennaCombinableExponent, packetInterval, packetSize, packetResourceCost) and
                                                            ModuleKerbNetAccess (FoV bounds, EnhancedSituationMask, AnomalyDetection, ...)
  harness/golden/serialized/celestial_bodies.serialized.json  the 17 stock bodies: Radius, atmosphere, scaledRadiusHorizonMultiplier,
                                                            scaledElipRadMult
The CommNetParams / GameVariables numbers are the defaults and presets of the decompile (cited next to each use). Geometry traces are drawn from
a seeded numpy generator and stored in the output so the C++ test replays exactly the values the reference saw.

Usage: python -I core/tools/wp39/commnet_reference.py > core/tests/data/wp39_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.abspath(os.path.join(CORE, ".."))
if not os.path.isdir(os.path.join(ROOT, "harness")):  # worktree: the shared harness lives in the main checkout
    ROOT = r"C:\Projects\KSP-UE5"
F = np.float32
DBL_MAX = sys.float_info.max


def st(x):
    """Mono float store: round a double expression to float32 (kept as a Python float)."""
    return float(F(x))


# ============================================================================== cfg parsing (original game data)

def parse_cfg(text):
    """Minimal KSP ConfigNode parser: returns the list of top-level (name, node) with node = {"values": [(k, v)], "nodes": [(name, node)]}."""
    root = {"values": [], "nodes": []}
    stack = [root]
    pending = None
    for raw in text.splitlines():
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line.endswith("{") and "=" not in line:
            if line != "{":
                pending = line[:-1].strip()
            node = {"values": [], "nodes": []}
            stack[-1]["nodes"].append((pending, node))
            stack.append(node)
            pending = None
        elif line == "}":
            stack.pop()
        elif "=" in line:
            k, v = line.split("=", 1)
            stack[-1]["values"].append((k.strip(), v.strip()))
        else:
            pending = line
    return root


def val(node, key, default=None):
    r = default
    for k, v in node["values"]:
        if k == key:
            r = v
    return r


def part_cfgs():
    base = os.path.join(CORE, "testdata", "GameData-mirror", "GameData")
    out = []
    for dp, _dn, fn in os.walk(base):
        for f in sorted(fn):
            if f.endswith(".cfg"):
                out.append(os.path.join(dp, f))
    return sorted(out)


def captured_parts():
    """The parts the game really loads: the `parts` table of the in-game capture C9 (harness/captures/out2/part_database.json). The mirror also
    holds cfgs of parts that are not in the database (deprecated / hidden duplicates)."""
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "part_database.json")))
    t = [x for x in d["tables"] if x["name"] == "parts"][0]
    cols = [c["name"] for c in t["columns"]]
    return {r[0]: r[cols.index("cfg_file")] for r in t["rows"]}


def stock_modules():
    """(partName, moduleNode) for every MODULE of a PART in the mirror that is a part of the game's database, in file order."""
    res = []
    real = captured_parts()
    for path in part_cfgs():
        try:
            txt = open(path, encoding="utf-8-sig", errors="replace").read()
        except OSError:
            continue
        if "ModuleDataTransmitter" not in txt and "ModuleKerbNetAccess" not in txt:
            continue
        root = parse_cfg(txt)
        for name, node in root["nodes"]:
            if name != "PART":
                continue
            pname = val(node, "name").replace("_", ".")
            rel = os.path.relpath(path, os.path.join(CORE, "testdata", "GameData-mirror")).replace("\\", "/")
            if real.get(pname) != rel:   # not in the database, or a duplicate cfg of a part another file defines
                continue
            for mname, m in node["nodes"]:
                if mname == "MODULE":
                    res.append((pname, m))
    return res


# ============================================================================== DoubleCurve (DoubleCurve.cs:70 Evaluate)

class DoubleCurve:
    def __init__(self, keys):
        self.keys = keys  # (time, value, inTangent, outTangent)
        self.minTime = min(k[0] for k in keys)
        self.maxTime = max(k[0] for k in keys)

    def evaluate(self, t):
        keys = self.keys
        num = len(keys) - 1
        num2 = 0
        if t < self.minTime:
            return keys[num2][1]
        if t > self.maxTime:
            return keys[num][1]
        while True:
            if num - num2 > 1:
                num3 = (num + num2) // 2
                time = keys[num3][0]
                if time == t:
                    return keys[num3][1]
                if time < t:
                    num2 = num3
                else:
                    num = num3
                continue
            k0 = keys[num2]
            k1 = keys[num]
            num4 = k1[0] - k0[0]
            num5 = k0[3] * num4
            num6 = k1[2] * num4
            t = (t - k0[0]) / num4
            num7 = t * t
            num8 = num7 * t
            num9 = 2.0 * num8 - 3.0 * num7 + 1.0
            num10 = num8 - 2.0 * num7 + t
            num11 = num8 - num7
            num12 = -2.0 * num8 + 3.0 * num7
            return num9 * k0[1] + num10 * num5 + num11 * num6 + num12 * k1[1]


RANGE_CURVE = DoubleCurve([(0.0, 0.0, 0.0, 0.0), (1.0, 1.0, 0.0, 0.0)])  # ModuleDataTransmitter.OnAwake (:111), GameVariables.GetDSNRangeCurve (:537)


# ============================================================================== .NET Dictionary<K,V> enumeration order

class NetDict:
    """Dictionary<TKey,TValue> as far as the enumeration order goes: entries are appended; Remove puts the slot on a LIFO free list that the next
    Add reuses; Clear resets both."""

    def __init__(self):
        self.entries = []   # None = free slot, else [key, value]
        self.free = []
        self.index = {}

    def count(self):
        return len(self.index)

    def get(self, k):
        i = self.index.get(id(k))
        return None if i is None else self.entries[i][1]

    def has(self, k):
        return id(k) in self.index

    def add(self, k, v):
        if self.free:
            i = self.free.pop()
            self.entries[i] = [k, v]
        else:
            i = len(self.entries)
            self.entries.append([k, v])
        self.index[id(k)] = i

    def remove(self, k):
        i = self.index.pop(id(k), None)
        if i is None:
            return False
        self.entries[i] = None
        self.free.append(i)
        return True

    def clear(self):
        self.entries = []
        self.free = []
        self.index = {}

    def items(self):
        return [(e[0], e[1]) for e in self.entries if e is not None]


# ============================================================================== CommNet (CommNode / CommLink / CommPath / CommNetwork)

def convert_signal(s):  # NodeUtilities.cs:5
    if s > 0.75:
        return 4
    if s > 0.5:
        return 3
    if s > 0.25:
        return 2
    if s > 1e-09:
        return 1
    return 0


def fdiv(a, b):
    """IEEE-754 double division (C# never throws): x/0 = +-inf, 0/0 = NaN."""
    with np.errstate(all="ignore"):
        return float(np.float64(a) / np.float64(b))


def in_range(a, b, sqr):  # CommRangeModel.cs:7
    num = a * b
    if sqr <= num:
        return num > 0.0
    return False


def normalized_range(a, b, d):  # :17
    return 1.0 - fdiv(d, math.sqrt(a * b))


class Node:
    def __init__(self, name, relay=0.0, transmit=0.0, home=False, control=False, multihop=True, pos=(0.0, 0.0, 0.0), offset=0.0):
        self.name = name
        self.links = NetDict()
        self.relay = relay
        self.transmit = transmit
        self.isHome = home
        self.isControlSource = control
        self.isControlSourceMultiHop = multihop
        self.pos = pos
        self.distanceOffset = offset
        self.occluder = None
        self.bestCost = 0.0
        self.bestLink = None
        self.bestLinkNode = None
        self.isInCandidateList = False
        self.pathingID = 0
        self.multiplier = 1.0

    def signal_multiplier(self, other):
        return self.multiplier


class Link:
    def __init__(self):
        self.a = self.b = None
        self.cost = 0.0
        self.pathingID = 0
        self.sS = 0.0
        self.strengthRR = self.strengthAR = self.strengthBR = 0.0
        self.aCanRelay = self.bCanRelay = self.bothRelay = False

    def update(self, ss):   # CommLink.cs:46 (sets signalStrength, not cost)
        self.sS = ss

    def set3(self, a, b, cost):  # Link.cs:21
        self.a, self.b = a, b
        self.update(cost)
        self.cost = cost

    def set4(self, a, b, distance, ss):  # CommLink.cs:38
        self.a, self.b = a, b
        self.cost = distance
        self.update(ss)

    def get_signal(self, aMust, bMust):  # CommLink.cs:52
        if aMust:
            if not bMust and self.strengthRR <= self.strengthAR:
                if self.strengthAR == 0.0:
                    return self.strengthBR
                return self.strengthAR
            return self.strengthRR
        if self.strengthRR > self.strengthBR:
            return self.strengthRR
        if self.strengthBR == 0.0:
            return self.strengthAR
        return self.strengthBR

    def get_signal_for(self, start):  # :77
        return self.get_signal(self.a is not start, self.b is not start)


class Occ:
    """OccluderHorizonCulling (radius X/Y/Z, rotation identity) of a body at `position`."""

    def __init__(self, position, rx, ry, rz):
        self.position = position
        self.anyZero = not (rx != 0.0 and ry != 0.0 and rz != 0.0)
        if not self.anyZero:
            self.rxr, self.ryr, self.rzr = 1.0 / rx, 1.0 / ry, 1.0 / rz
            self.radius = max(max(rx, ry), rz)

    def in_range(self, source, distance):  # Occluder.cs:9 / OccluderHorizonCulling.cs:46
        if self.anyZero:
            return False
        num = distance + self.radius
        num *= num
        d = [source[i] - self.position[i] for i in range(3)]
        return d[0] * d[0] + d[1] * d[1] + d[2] * d[2] <= num

    def raycast(self, source, dest):  # OccluderHorizonCulling.cs:55
        if self.anyZero:
            return False
        s = [source[i] - self.position[i] for i in range(3)]
        d = [dest[i] - self.position[i] for i in range(3)]
        sq = s[0] * s[0] + s[1] * s[1] + s[2] * s[2]
        sq2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2]
        if sq > sq2:
            s, d = d, s
        # invRotation is the identity (QuaternionD.Inverse(identity) * v == v exactly)
        s = [s[0] * self.rxr, s[1] * self.ryr, s[2] * self.rzr]
        d = [d[0] * self.rxr, d[1] * self.ryr, d[2] * self.rzr]
        num = s[0] * s[0] + s[1] * s[1] + s[2] * s[2] - 1.0
        lhs = [d[0] - s[0], d[1] - s[1], d[2] - s[2]]
        num2 = 0.0 - (lhs[0] * s[0] + lhs[1] * s[1] + lhs[2] * s[2])
        if num < 0.0:
            return num2 > 0.0
        sq3 = lhs[0] * lhs[0] + lhs[1] * lhs[1] + lhs[2] * lhs[2]
        if num2 > num and fdiv(num2 * num2, sq3) > num:
            return True
        return False


class Net:
    def __init__(self):
        self.nodes = []
        self.links = []
        self.occluders = []
        self.pathingID = -2 ** 31
        self.candidates = []
        self.isDirty = False

    # ---- Net.cs
    def add(self, n):
        if n not in self.nodes:
            self.nodes.append(n)
        n.links.clear()
        self.isDirty = True
        return n

    def remove(self, n):
        for _other, link in n.links.items():
            self.disconnect(n, link.b if link.a is n else link.a, False)
        n.links.clear()
        if n in self.nodes:
            self.nodes.remove(n)
        self.isDirty = True
        return True

    def connect(self, a, b, distance):
        v = a.links.get(b)
        if v is not None:
            v.update(distance)
        else:
            v = Link()
            v.set3(a, b, distance)
            a.links.add(b, v)
            b.links.add(a, v)
            self.links.append(v)
        return v

    def disconnect(self, a, b, removeFromA=True):
        if a is None or b is None:
            return
        v = a.links.get(b)
        if v is None:
            v = b.links.get(a)
        if v is not None:
            b.links.remove(a)
            if removeFromA:
                a.links.remove(b)
            if v in self.links:
                self.links.remove(v)

    def rebuild(self):
        self.isDirty = False
        count = len(self.nodes)
        for i in range(count - 1):
            for j in range(i + 1, count):
                self.set_node_connection(self.nodes[i], self.nodes[j])

    # ---- CommNetwork.cs
    def try_connect(self, a, b, distance, aCan, bCan, both):
        flag = False
        num = a.signal_multiplier(b) * b.signal_multiplier(a)
        if both:
            nr = normalized_range(a.relay, b.relay, distance)
            if nr > 0.0:
                num2 = math.sqrt(RANGE_CURVE.evaluate(nr) * RANGE_CURVE.evaluate(nr))
                num2 *= num
                if num2 > 0.0:
                    flag = True
            else:
                both = False
                num2 = 0.0
        else:
            num2 = 0.0
        if aCan:
            nr2 = normalized_range(a.relay, b.transmit, distance)
            if nr2 > 0.0:
                num3 = math.sqrt(RANGE_CURVE.evaluate(nr2) * RANGE_CURVE.evaluate(nr2))
                num3 *= num
                if num3 > 0.0:
                    flag = True
            else:
                aCan = False
                num3 = 0.0
        else:
            aCan = False
            num3 = 0.0
        if bCan:
            nr3 = normalized_range(a.transmit, b.relay, distance)
            if nr3 > 0.0:
                num4 = math.sqrt(RANGE_CURVE.evaluate(nr3) * RANGE_CURVE.evaluate(nr3))
                num4 *= num
                if num4 > 0.0:
                    flag = True
            else:
                bCan = False
                num4 = 0.0
        else:
            bCan = False
            num4 = 0.0
        if flag:
            link = self.connect(a, b, distance)
            link.strengthRR, link.strengthAR, link.strengthBR = num2, num3, num4
            link.aCanRelay, link.bCanRelay, link.bothRelay = aCan, bCan, both
            return True
        self.disconnect(a, b)
        return False

    def test_occlusion(self, aPos, a, bPos, b, distance):
        for occ in reversed(self.occluders):
            if occ is a or occ is b or not occ.in_range(aPos, distance) or not occ.raycast(aPos, bPos):
                continue
            return False
        return True

    def set_node_connection(self, a, b):
        if a.isHome and b.isHome:
            self.disconnect(a, b)
            return False
        if a.relay + a.transmit != 0.0 and b.relay + b.transmit != 0.0:
            pa, pb = a.pos, b.pos
            d = [pb[i] - pa[i] for i in range(3)]
            num = d[0] * d[0] + d[1] * d[1] + d[2] * d[2]
            num2 = a.distanceOffset + b.distanceOffset
            if num2 != 0.0:
                num2 = math.sqrt(num) + num2
                if num2 > 0.0:
                    num = num2 * num2
                else:
                    num2 = 0.0
                    num = 0.0
            flag = in_range(a.relay, b.relay, num)
            flag2 = flag
            flag3 = flag
            if not flag:
                flag2 = in_range(a.relay, b.transmit, num)
                flag3 = in_range(a.transmit, b.relay, num)
            if not flag2 and not flag3:
                self.disconnect(a, b)
                return False
            if num == 0.0 and (flag or flag2 or flag3):
                return self.try_connect(a, b, 1e-07, flag2, flag3, flag)
            if num2 == 0.0:
                num2 = math.sqrt(num)
            if self.test_occlusion(pa, a.occluder, pb, b.occluder, num2):
                return self.try_connect(a, b, num2, flag2, flag3, flag)
            self.disconnect(a, b)
            return False
        self.disconnect(a, b)
        return False

    def update_shortest(self, a, b, link, bestCost, start, where=None):
        num = link.get_signal_for(start)
        num2 = bestCost * num
        if num > 0.0 and num2 > b.bestCost:
            b.bestCost = num2
            b.bestLinkNode = a
            b.bestLink = link
            if not b.isInCandidateList and b.pathingID != self.pathingID:
                self.candidates.append(b)
                b.isInCandidateList = True
                b.pathingID = self.pathingID
            if where is not None:
                return b if where(start, b) else None
        return None

    def _reset(self, start):
        self.pathingID += 1
        for n in reversed(self.nodes):
            n.bestCost = -DBL_MAX
            n.bestLink = None
            n.bestLinkNode = None
            n.isInCandidateList = False
        self.candidates = []
        self.candidates.append(start)
        start.bestCost = 1.0
        start.isInCandidateList = True
        start.pathingID = self.pathingID

    def _path_from(self, end, start):
        path = []
        n = end
        if n.bestLinkNode is not None:
            while n is not start:
                b = n
                bl = n.bestLink
                n = n.bestLinkNode
                lk = Link()
                lk.set4(n, b, bl.sS, bl.get_signal_for(start))
                path.insert(0, lk)
        return path

    def find_path(self, start, end):
        if end is None or start is end:
            return None
        if self.isDirty:
            self.rebuild()
        self._reset(start)
        while self.candidates:
            n = self.candidates.pop(0)
            n.isInCandidateList = False
            for k, link in n.links.items():
                self.update_shortest(n, k, link, n.bestCost, start)
        path = self._path_from(end, start)
        if not path:
            return None
        if not (path[0].a is start or path[0].b is start):
            return None
        return path

    def find_closest_where(self, start, where):
        if start is None:
            return None, None
        if self.isDirty:
            self.rebuild()
        num = -DBL_MAX
        best = None
        self._reset(start)
        while self.candidates:
            n = self.candidates.pop(0)
            n.isInCandidateList = False
            for k, link in n.links.items():
                if link.pathingID != self.pathingID:
                    link.pathingID = self.pathingID
                    r = self.update_shortest(n, k, link, n.bestCost, start, where)
                    if r is not None and r.bestCost > num:
                        best = r
                        num = r.bestCost
        if best is None:
            return None, None
        path = self._path_from(best, start)
        if not (path[0].a is start or path[0].b is start):
            return None, None
        return best, path


def is_home(start, a):
    return a.isHome


def is_control(start, a):
    if not a.isControlSourceMultiHop:
        if a.isControlSource:
            return a.links.has(start)
        return False
    return a.isControlSource


def path_summary(path):
    """CommPath.UpdateFromPath: product of the hop signalStrength, the hop list, the signal enum, the last hop type."""
    if not path:
        return None
    s = 1.0
    for lk in path:
        s *= lk.sS
    last = path[-1]
    return {"nodes": [path[0].a.name] + [lk.b.name for lk in path], "costs": [lk.cost for lk in path], "hopStrength": [lk.sS for lk in path],
            "signalStrength": s, "signal": convert_signal(s), "lastHopHome": bool(last.b.isHome)}


# ============================================================================== stock data

def antennas_from_cfgs():
    out = []
    for pname, m in stock_modules():
        if val(m, "name") != "ModuleDataTransmitter":
            continue
        out.append({
            "part": pname,
            "type": val(m, "antennaType", "DIRECT"),
            "power": float(val(m, "antennaPower", "500000.0")),
            "combinable": val(m, "antennaCombinable", "False").lower() == "true",
            "exponent": float(val(m, "antennaCombinableExponent", "0.75")),
            "packetInterval": st(float(val(m, "packetInterval", "0.5"))),
            "packetSize": st(float(val(m, "packetSize", "1"))),
            "packetResourceCost": float(val(m, "packetResourceCost", "10.0")),
            "hasDeployFx": val(m, "DeployFxModules") is not None,
        })
    return out


SITUATIONS = [("LANDED", 1), ("SPLASHED", 2), ("PRELAUNCH", 4), ("FLYING", 8), ("SUB_ORBITAL", 16), ("ORBITING", 32), ("ESCAPING", 64), ("DOCKED", 128)]


def kerbnet_from_cfgs():
    out = []
    for pname, m in stock_modules():
        if val(m, "name") != "ModuleKerbNetAccess":
            continue
        d = {
            "part": pname,
            "MinimumFoV": st(float(val(m, "MinimumFoV", "5"))),
            "MaximumFoV": st(float(val(m, "MaximumFoV", "90"))),
            "EnhancedSituationMask": int(val(m, "EnhancedSituationMask", "0")),
            "EnhancedMinimumFoV": st(float(val(m, "EnhancedMinimumFoV", "5"))),
            "EnhancedMaximumFoV": st(float(val(m, "EnhancedMaximumFoV", "90"))),
            "AnomalyDetection": st(float(val(m, "AnomalyDetection", "0.25"))),
            "RequiresAnimation": val(m, "RequiresAnimation", "False").lower() == "true",
        }
        modes = []
        for nm, n in m["nodes"]:
            if nm == "DISPLAY_MODES":
                for k, v in n["values"]:
                    if k == "Mode" and v:
                        mode = v.split(",")[0].strip()
                        if mode not in modes:
                            modes.append(mode)
        effects = []
        for nm, n in m["nodes"]:
            if nm == "REQUIRED_EFFECTS":
                for k, v in n["values"]:
                    if k == "Effect":
                        t = v.strip()
                        if t and t not in effects:
                            effects.append(t)
        d["modes"] = modes
        d["effects"] = effects
        # the situation walk of CheckVesselSituationFoV (:356) over every single situation, firstRun then change events
        rows = []
        for firstRun in (True, False):
            for name, bit in SITUATIONS:
                for prev in (False, True):
                    is_enh = prev
                    flag2 = (bit & d["EnhancedSituationMask"]) != 0
                    flag3 = False
                    mn = mx = 0.0
                    if (not is_enh or firstRun) and flag2:
                        is_enh = True
                        flag3 = True
                        mn, mx = d["EnhancedMinimumFoV"], d["EnhancedMaximumFoV"]
                    elif (is_enh or firstRun) and not flag2:
                        is_enh = False
                        flag3 = True
                        mn, mx = d["MinimumFoV"], d["MaximumFoV"]
                    rows.append({"firstRun": firstRun, "situation": bit, "prevEnhanced": prev, "enhanced": is_enh, "boundsSet": flag3, "min": mn, "max": mx,
                                 "getMin": d["EnhancedMinimumFoV"] if is_enh else d["MinimumFoV"],
                                 "getMax": d["EnhancedMaximumFoV"] if is_enh else d["MaximumFoV"]})
        d["rows"] = rows
        d["enhancedSituations"] = [bit for name, bit in SITUATIONS if bit & d["EnhancedSituationMask"]]
        out.append(d)
    return out


def bodies():
    p = os.path.join(ROOT, "harness", "golden", "serialized", "celestial_bodies.serialized.json")
    d = json.load(open(p))
    res = []
    for b in d["bodies"]:
        cb = b["celestialBody"]
        res.append({"name": cb["bodyName"], "Radius": cb["Radius"], "atmosphere": bool(cb["atmosphere"]),
                    "horizon": cb["scaledRadiusHorizonMultiplier"], "elip": [cb["scaledElipRadMult"]["x"], cb["scaledElipRadMult"]["y"], cb["scaledElipRadMult"]["z"]]})
    return res


# CommNetParams (CommNetParams.cs:15-24 defaults; SetDifficultyPreset :46-70). DSNModifier default 1.
PRESETS = {
    "Default": {"rangeModifier": 1.0, "occVac": 0.9, "occAtm": 0.75},
    "Easy": {"rangeModifier": 1.5, "occVac": 0.0, "occAtm": 0.0},
    "Moderate": {"rangeModifier": 0.8, "occVac": 1.0, "occAtm": 0.85},
    "Hard": {"rangeModifier": 0.65, "occVac": 1.0, "occAtm": 1.0},
}


def dsn_range(level, modifier=1.0):  # GameVariables.cs:562
    num = 250000000000.0 if level > 0.500000001 else (50000000000.0 if level > 1e-9 else 2000000000.0)
    return num * st(modifier)


# ============================================================================== sections

def section_dsn():
    return [{"level": lv, "modifier": m, "range": dsn_range(lv, m)} for lv in (0.0, 0.5, 1.0) for m in (1.0, 0.5, 2.0)]


def section_range_grid(ants):
    powers = sorted({a["power"] for a in ants} | {dsn_range(lv) for lv in (0.0, 0.5, 1.0)})
    rows = []
    for ap in powers:
        for bp in powers:
            m = math.sqrt(ap * bp)
            for f in (0.0, 0.01, 0.1, 0.5, 0.9, 0.999999, 1.0, 1.0001, 1.5):
                dist = f * m
                rows.append({"a": ap, "b": bp, "distance": dist, "sqr": dist * dist, "inRange": in_range(ap, bp, dist * dist), "norm": normalized_range(ap, bp, dist) if m > 0 else 0.0,
                             "max": m})
    return {"powers": powers, "rows": rows}


def node_configs(ants):
    """(relay, transmit) pairs of vessels / stations drawn from the stock antennas and DSN levels."""
    direct = sorted({a["power"] for a in ants if a["type"] == "DIRECT"})
    relay = sorted({a["power"] for a in ants if a["type"] == "RELAY"})
    dsn = [dsn_range(lv) for lv in (0.0, 0.5, 1.0)]
    cfgs = []
    cfgs.append((0.0, direct[0]))
    cfgs.append((0.0, direct[-1]))
    cfgs.append((relay[0], direct[0]))
    cfgs.append((relay[-1], direct[-1]))
    cfgs.append((relay[-1], 0.0))
    cfgs.append((direct[-1], 0.0))
    for d in dsn:
        cfgs.append((d, 0.0))
    return cfgs


def section_signals(ants):
    cfgs = node_configs(ants)
    rows = []
    for ca in cfgs:
        for cb in cfgs:
            m = math.sqrt(max(ca[0] * cb[0], ca[0] * cb[1], ca[1] * cb[0], 1.0))
            for f in (0.0, 0.05, 0.3, 0.5, 0.75, 0.95, 0.999, 1.0, 1.001):
                for offs in ((0.0, 0.0), (1000.0, -250.0), (-2.0 * m, 0.0)):
                    dist = f * m
                    net = Net()
                    a = Node("a", ca[0], ca[1], pos=(0.0, 0.0, 0.0), offset=offs[0])
                    b = Node("b", cb[0], cb[1], pos=(dist, 0.0, 0.0), offset=offs[1])
                    net.add(a)
                    net.add(b)
                    net.rebuild()
                    lk = a.links.get(b)
                    r = {"aRelay": ca[0], "aTransmit": ca[1], "bRelay": cb[0], "bTransmit": cb[1], "distance": dist, "offA": offs[0], "offB": offs[1],
                         "connected": lk is not None}
                    if lk is not None:
                        r.update({"cost": lk.cost, "sS": lk.sS, "rr": lk.strengthRR, "ar": lk.strengthAR, "br": lk.strengthBR, "aCan": lk.aCanRelay,
                                  "bCan": lk.bCanRelay, "both": lk.bothRelay,
                                  "sig": [lk.get_signal(x, y) for x in (False, True) for y in (False, True)],
                                  "signalEnum": convert_signal(lk.sS),
                                  "best": max(max(lk.strengthAR, lk.strengthBR), lk.strengthRR)})
                    rows.append(r)
    return rows


def occluder_for(body, preset, position):
    p = PRESETS[preset]
    mult = p["occAtm"] if body["atmosphere"] else p["occVac"]
    num = body["Radius"] * body["horizon"] * st(mult)  # CommNetBody.cs:40
    return Occ(position, body["elip"][0] * num, body["elip"][1] * num, body["elip"][2] * num), num


def section_occlusion(bs):
    rng = np.random.default_rng(39)
    rows = []
    for body in bs:
        for preset in PRESETS:
            bpos = [float(x) for x in rng.uniform(-3.0e10, 3.0e10, size=3)]
            occ, num = occluder_for(body, preset, tuple(bpos))
            geo = []
            for _ in range(24):
                u1 = rng.normal(size=3)
                u1 /= np.linalg.norm(u1)
                u2 = rng.normal(size=3)
                u2 /= np.linalg.norm(u2)
                k1 = float(rng.choice([1.0001, 1.5, 3.0, 10.0, 100.0]))
                k2 = float(rng.choice([1.0001, 1.5, 3.0, 10.0, 100.0]))
                s = [bpos[i] + float(u1[i]) * body["Radius"] * k1 for i in range(3)]
                d = [bpos[i] + float(u2[i]) * body["Radius"] * k2 for i in range(3)]
                dd = [d[i] - s[i] for i in range(3)]
                dist = math.sqrt(dd[0] * dd[0] + dd[1] * dd[1] + dd[2] * dd[2])
                geo.append({"s": s, "d": d, "dist": dist, "blocked": bool(occ.raycast(s, d)), "inRange": bool(occ.in_range(s, dist))})
            rows.append({"body": body["name"], "preset": preset, "bodyPos": bpos, "radius": occ.radius if not occ.anyZero else 0.0, "occRadiusBase": num, "geo": geo})
    return rows


def snapshot_links(net):
    return {n.name: [k.name for k, _ in n.links.items()] for n in net.nodes}


def build_scenario(spec):
    net = Net()
    nodes = {}
    for s in spec["nodes"]:
        n = Node(s["name"], s["relay"], s["transmit"], s["home"], s["control"], s["multihop"], tuple(s["pos"]))
        nodes[s["name"]] = n
    return net, nodes


def run_queries(net, nodes, queries):
    res = []
    for q in queries:
        start = nodes[q["from"]]
        if q["kind"] == "home":
            best, path = net.find_closest_where(start, is_home)
            res.append({"q": q, "found": best is not None, "target": best.name if best else None, "path": path_summary(path)})
        elif q["kind"] == "control":
            best, path = net.find_closest_where(start, is_control)
            res.append({"q": q, "found": best is not None, "target": best.name if best else None, "path": path_summary(path)})
        else:
            path = net.find_path(start, nodes[q["to"]])
            res.append({"q": q, "found": path is not None, "target": q["to"], "path": path_summary(path)})
    return res


def section_graphs(ants):
    rng = np.random.default_rng(3939)
    direct = sorted({a["power"] for a in ants if a["type"] == "DIRECT"})
    relay = sorted({a["power"] for a in ants if a["type"] == "RELAY"})
    dsn0, dsn1, dsn2 = dsn_range(0.0), dsn_range(0.5), dsn_range(1.0)
    scenarios = []

    # S1: random constellation around a planet-sized shell with homes, relays and vessels
    spec = {"nodes": []}
    names = []

    def add(name, relay_p, transmit_p, home=False, control=False, multihop=True, pos=(0, 0, 0)):
        spec["nodes"].append({"name": name, "relay": relay_p, "transmit": transmit_p, "home": home, "control": control, "multihop": multihop, "pos": list(pos)})

    def shell(r):
        u = rng.normal(size=3)
        u /= np.linalg.norm(u)
        return [float(x) * r for x in u]

    add("KSC", dsn1, 0.0, True, True, True, [0.0, 0.0, 600000.0])
    add("Outpost", dsn0, 0.0, True, True, True, [600000.0, 0.0, 0.0])
    for i in range(10):
        rp = relay[int(rng.integers(0, len(relay)))] if i % 2 == 0 else 0.0
        tp = direct[int(rng.integers(0, len(direct)))]
        add("V%d" % i, rp, tp, False, i % 3 == 0, i % 2 == 0, shell(float(10.0 ** rng.uniform(math.log10(7.0e5), math.log10(4.0e9)))))
    add("Probe", 0.0, direct[0], False, False, False, shell(2.0e9))
    scenarios.append(("random", spec))

    # S2: equal-strength lattice (ties decided only by the dictionary order)
    spec2 = {"nodes": []}
    spec = spec2

    def add2(name, r, t, home, control, mh, pos):
        spec2["nodes"].append({"name": name, "relay": r, "transmit": t, "home": home, "control": control, "multihop": mh, "pos": list(pos)})
    P = relay[0]
    step = math.sqrt(P * P) * 0.25
    add2("H", dsn2, 0.0, True, True, True, [0.0, 0.0, 0.0])
    k = 0
    for x in range(3):
        for y in range(3):
            if x == 0 and y == 0:
                continue
            add2("L%d%d" % (x, y), P, 0.0, False, False, True, [x * step, y * step, 0.0])
            k += 1
    add2("Far", 0.0, direct[0], False, False, False, [2 * step, 2 * step, step * 0.5])
    scenarios.append(("lattice", spec2))

    out = []
    for name, sp in scenarios:
        net, nodes = build_scenario(sp)
        for s in sp["nodes"]:
            net.add(nodes[s["name"]])
        net.rebuild()
        allnames = [s["name"] for s in sp["nodes"]]
        queries = []
        for n in allnames:
            queries.append({"kind": "home", "from": n})
            queries.append({"kind": "control", "from": n})
        for i in range(0, len(allnames), 2):
            for j in range(len(allnames)):
                if i != j:
                    queries.append({"kind": "path", "from": allnames[i], "to": allnames[j]})
        phases = []
        phases.append({"label": "built", "ops": [], "links": snapshot_links(net), "results": run_queries(net, nodes, queries)})
        # phase 2: remove one node and add it back (free-list order), then move a node out of range and back
        victim = allnames[3]
        net.remove(nodes[victim])
        net.rebuild()
        l1 = snapshot_links(net)
        r1 = run_queries(net, {k: v for k, v in nodes.items() if k != victim}, [q for q in queries if q.get("from") != victim and q.get("to") != victim])
        phases.append({"label": "removed", "ops": [{"op": "remove", "node": victim}], "links": l1, "results": r1,
                       "queries": [q for q in queries if q.get("from") != victim and q.get("to") != victim]})
        net.add(nodes[victim])
        net.rebuild()
        phases.append({"label": "re-added", "ops": [{"op": "add", "node": victim}], "links": snapshot_links(net), "results": run_queries(net, nodes, queries)})
        mover = allnames[4]
        old = nodes[mover].pos
        nodes[mover].pos = (1.0e13, 1.0e13, 1.0e13)
        net.rebuild()
        phases.append({"label": "moved-away", "ops": [{"op": "move", "node": mover, "pos": [1.0e13, 1.0e13, 1.0e13]}], "links": snapshot_links(net),
                       "results": run_queries(net, nodes, queries)})
        nodes[mover].pos = old
        net.rebuild()
        phases.append({"label": "moved-back", "ops": [{"op": "move", "node": mover, "pos": list(old)}], "links": snapshot_links(net),
                       "results": run_queries(net, nodes, queries)})
        out.append({"name": name, "spec": sp, "queries": queries, "phases": phases})
    return out


# ---------------------------------------------------------------------------- CommNetVessel.UpdateComm (antenna aggregation)

def update_comm(antennas, relay_enabler, range_modifier):
    """CommNetVessel.cs:218. `antennas` is the vessel's list of usable antennas in module order (part index descending, module index descending is
    the C# visit order; the caller passes them already in visit order)."""
    commAntenna2 = None
    num2 = num3 = num4 = num5 = 0.0
    commAntenna3 = None
    num6 = num7 = num8 = num9 = 0.0
    commAntenna4 = commAntenna5 = None
    hasScience = False
    flag = bool(relay_enabler)
    for ant in antennas:
        num10 = ant["power"]
        if ant["type"] != "RELAY":
            if ant["type"] != "INTERNAL":
                hasScience = True
            if num10 > num2:
                commAntenna2 = ant
                num2 = num10
            if ant["combinable"]:
                num3 += num10
                num4 += num10 * ant["exponent"]
                if commAntenna4 is None or num10 > num5:
                    commAntenna4 = ant
                    num5 = num10
            continue
        hasScience = True
        if num10 > num6:
            commAntenna3 = ant
            num6 = num10
            if commAntenna3["combinable"]:
                commAntenna5 = ant
        if ant["combinable"]:
            num7 += num10
            num8 += num10 * ant["exponent"]
            if commAntenna5 is None or num10 > num9:
                commAntenna5 = ant
                num9 = num10
    tpower = 0.0
    tcomb = False
    rpower = 0.0
    rcomb = False
    if commAntenna2 is not None:
        num11 = num3 + num7
        flag2 = num5 > num9
        if num11 > 0.0:
            num12 = num5 if flag2 else num9
            if num11 != num12:
                y = (num4 + num8) / num11
                num11 = num12 * math.pow(num11 / num12, y)
            if num7 > 0.0 and num7 != num9:
                num7 = num9 * math.pow(num7 / num9, num8 / num7)
        if num3 > 0.0 and num11 > num2:
            tcomb = True
        else:
            num11 = num2
            tcomb = False
        tpower = num11
    if commAntenna3 is not None:
        if num7 > num6:
            num13 = num7
            rcomb = True
        else:
            num13 = num6
            rcomb = False
        rpower = num13
    if flag and tpower > rpower:
        rpower = tpower
        rcomb = tcomb
        tpower = 0.0
    tpower *= st(range_modifier)
    rpower *= st(range_modifier)
    return {"transmit": tpower, "relay": rpower, "tcomb": tcomb, "rcomb": rcomb, "hasScience": hasScience}


def section_update_comm(ants):
    rng = np.random.default_rng(390)
    real = [a for a in ants]
    rows = []
    for t in range(240):
        k = int(rng.integers(1, 6))
        idx = [int(rng.integers(0, len(real))) for _ in range(k)]
        relay_en = bool(rng.integers(0, 2)) if t % 3 == 0 else False
        rm = [1.0, 1.5, 0.8, 0.65][t % 4]
        # antennas that can communicate: the visit order is the order listed here
        sel = [real[i] for i in idx]
        r = update_comm(list(reversed(sel)), relay_en, rm)  # the C# visits the parts from the last one down
        rows.append({"parts": [s["part"] for s in sel], "relayEnabler": relay_en, "rangeModifier": rm, "expect": r})
    return rows


# ---------------------------------------------------------------------------- control state

def level(cs):  # CommNetVessel.GetControlLevel (:555): FULL 3, PARTIAL_MANNED 2, PARTIAL_UNMANNED 1, NONE 0
    if cs & 16 > 0:
        return 3
    if cs & 8 > 0:
        return 2 if cs & 4 > 0 else 1
    return 0


def section_control_states():
    states = [0, 2, 4, 8, 10, 12, 16, 18, 20]
    levels = [{"state": s, "level": level(s)} for s in states]
    rng = np.random.default_rng(3900)
    combos = []
    for _ in range(200):
        k = int(rng.integers(0, 6))
        src = [{"state": states[int(rng.integers(0, len(states)))], "comm": bool(rng.integers(0, 2))} for _ in range(k)]
        cs = 0
        cc = False
        for s in src:
            if s["state"] > cs:
                cs = s["state"]
            cc = cc or s["comm"]
        combos.append({"sources": src, "controlState": cs, "canComm": cc, "level": level(cs)})
    return {"levels": levels, "combos": combos}


# ---------------------------------------------------------------------------- transmission bookkeeping

def sim_transmit(ant, amount):
    """ModuleDataTransmitter.transmitQueuedData for one ScienceData with plenty of ElectricCharge (:358)."""
    size = F(ant["packetSize"])
    total = F(amount)
    packets = int(math.ceil(float(F(float(total) / float(size)))))
    total_packets = packets
    sent = []
    steps_per_packet = 0
    # WaitForFixedSeconds(interval): curTime += fixedDeltaTime (0.02f) until curTime >= seconds (float accumulation)
    cur = F(0.0)
    interval = F(ant["packetInterval"])
    dt = F(0.02)
    while cur < interval:
        steps_per_packet += 1
        cur = F(float(cur) + float(dt))
    cap = 0.0
    ec = 0.0
    pkt_cost = ant["packetResourceCost"]
    while packets > 0:
        num = pkt_cost - cap
        cap += num * 1.0   # UpdateModuleResourceInputs returns 1.0 with enough charge
        ec += num
        if cap >= pkt_cost * 0.95:
            cap = 0.0
            data_amount = F(min(float(size), float(total)))
            total = F(float(total) - float(size))
            packets -= 1
            sent.append(float(data_amount))
    return {"packets": total_packets, "sent": sent, "stepsPerPacket": steps_per_packet, "ec": ec,
            "rate": st(float(size) * (1.0 / float(F(interval)))), "dataResourceCost": pkt_cost / float(size)}


def section_transmit(ants):
    rows = []
    for a in ants:
        if a["type"] == "INTERNAL":
            continue
        for amount in (0.5, 1.0, 1.5, 5.0, 7.3, 10.0, 25.0, 33.3):
            r = sim_transmit(a, amount)
            r.update({"part": a["part"], "amount": amount})
            rows.append(r)
    return rows


def main():
    ants = antennas_from_cfgs()
    out = {
        "provenance": {"cfgs": "core/testdata/GameData-mirror/GameData (ModuleDataTransmitter / ModuleKerbNetAccess)",
                       "bodies": "harness/golden/serialized/celestial_bodies.serialized.json",
                       "rangeCurve": "DoubleCurve (0,0,0,0)-(1,1,0,0): ModuleDataTransmitter.cs:111, GameVariables.cs:537",
                       "spec": "CommNet/CommNetwork.cs, CommLink.cs, CommPath.cs, CommRangeModel.cs, Occluders/OccluderHorizonCulling.cs, CommNetVessel.cs"},
        "antennas": ants,
        "kerbnet": kerbnet_from_cfgs(),
        "presets": PRESETS,
        "dsn": section_dsn(),
        "rangeGrid": section_range_grid(ants),
        "signals": section_signals(ants),
        "occlusion": section_occlusion(bodies()),
        "graphs": section_graphs(ants),
        "updateComm": section_update_comm(ants),
        "control": section_control_states(),
        "transmit": section_transmit(ants),
    }
    json.dump(out, sys.stdout, separators=(",", ":"))


if __name__ == "__main__":
    main()
