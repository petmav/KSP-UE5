"""WP-54c reference, part 0: shared helpers (no kspcore code). Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/*.cs).
The synthetic world, localizer model and ConfigNode model are those of tools/wp54b (imported); this file adds the mission model the WP-54c scenarios need
(vessel situations by id, mapped vessels, the nodes) and the CelestialBody surface-position maths."""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(CORE, "tools", "wp54b"))
sys.path.insert(0, os.path.join(CORE, "tools", "wp55"))

from ref_common import *  # noqa: E402,F401,F403
from ref_common import Cfg, f32, fmt, tag, b, guid_of, BODIES, HOME, body, INF  # noqa: E402
from ref_data import D, F, FS, rng_pick, VGL, Area, great_circle, body_by_name_or_none, POOL_NUM, uint_try  # noqa: E402,F401
from ref_restrict import TICK  # noqa: E402,F401

OUT = os.path.join(CORE, "tests", "data", "wp54c_reference.json")   # after the star import: ref_common has its own OUT (wp54b)
EMPTY_GUID = "00000000-0000-0000-0000-000000000000"
DEG = math.pi / 180.0


# ---- Mission model: what the WP-54c scenarios read from Mission.cs -------------------------------------------------------------------
class MappedVessel:
    def __init__(self, part, orig, cur, pvn, craft, sit="", mapped_id=0):
        self.part = part          # partPersistentId
        self.orig = orig          # originalVesselPersistentId
        self.cur = cur            # currentVesselPersistentId
        self.pvn = pvn            # partVesselName
        self.craft = craft        # craftFileName
        self.sit = sit            # situationVesselName
        self.mapped = mapped_id   # mappedVesselPersistentId

    def copy(self):
        return MappedVessel(self.part, self.orig, self.cur, self.pvn, self.craft, self.sit, self.mapped)

    def to_json(self):
        return {"part": self.part, "orig": self.orig, "cur": self.cur, "pvn": self.pvn, "craft": self.craft, "sit": self.sit, "mapped": self.mapped}

    def dump(self):
        return [self.part, self.orig, self.cur, self.pvn, self.craft, self.sit, self.mapped]


def mv_index_part(lst, i):
    for k, m in enumerate(lst):
        if m.part == i:
            return k
    return -1


def mv_index_mapped(lst, i):
    for k, m in enumerate(lst):
        if m.mapped == i:
            return k
    return -1


def mv_index_current(lst, i):
    for k, m in enumerate(lst):
        if m.cur == i:
            return k
    return -1


def mv_convert(lst, i):
    k = mv_index_part(lst, i)
    if k != -1:
        return lst[k].cur
    k = mv_index_mapped(lst, i)
    if k != -1:
        return lst[k].cur
    return i


def mv_exists(lst, vsl, part, name, craft):
    for m in lst:
        if (m.cur == vsl or m.orig == vsl) and m.part == part and m.pvn == name and m.craft == craft:
            return True
    return False


def mv_find(lst, vsl, part, name, craft):
    for m in lst:
        if (m.cur == vsl or m.orig == vsl) and m.part == part and m.pvn == name and m.craft == craft:
            return m
    return None


class MissionModel:
    """vessels: list of {"pid","name","craft","node"} (ActionCreateVessel situations in node order); mapped: list of MappedVessel; flags / asteroids / comets
    by persistent id; nodes: {guid: title}"""

    def __init__(self):
        self.vessels = []
        self.mapped = []
        self.flags = []       # {"pid", "site"}
        self.asteroids = []   # {"pid", "name"}
        self.comets = []      # {"pid", "name"}
        self.nodes = {}       # guid -> title
        self.node_labels = {}  # guid -> label provider (callable returning world position or None)

    def vessel_by_id(self, pid):
        for v in self.vessels:
            if v["pid"] == pid:
                return v
        return None

    def get_vessel_situation(self, pid):
        return self.vessel_by_id(pid)

    def update_from_mapped(self, vid):
        """-> (found, vesselName, situation)"""
        k = mv_index_mapped(self.mapped, vid)
        if k == -1:
            return False, "", None
        return True, self.mapped[k].pvn, self.vessel_by_id(self.mapped[k].cur)

    def current_vessel_id(self, vid):
        # TestDistance scenarios: no part of the mapped vessels is registered in FlightGlobals, so the part fallbacks of CurrentVesselID find nothing
        return mv_convert(self.mapped, vid)

    def asteroid(self, pid):
        for a in self.asteroids:
            if a["pid"] == pid:
                return a
        return None

    def comet(self, pid):
        for a in self.comets:
            if a["pid"] == pid:
                return a
        return None

    def flag(self, pid):
        for a in self.flags:
            if a["pid"] == pid:
                return a
        return None

    def to_json(self):
        return {"vessels": self.vessels, "mapped": [m.to_json() for m in self.mapped], "flags": self.flags, "asteroids": self.asteroids, "comets": self.comets,
                "nodes": [[g, t] for g, t in self.nodes.items()]}


# ---- CelestialBody surface maths (CelestialBody.cs:GetWorldSurfacePosition, Planetarium.CelestialFrame) ----------------------------------
def celestial_frame(A, B, C):
    num, num2 = math.cos(A), math.sin(A)
    num3, num4 = math.cos(B), math.sin(B)
    num5, num6 = math.cos(C), math.sin(C)
    X = (num * num5 - num2 * num3 * num6, num2 * num5 + num * num3 * num6, num4 * num6)
    Y = ((0.0 - num) * num6 - num2 * num3 * num5, (0.0 - num2) * num6 + num * num3 * num5, num4 * num5)
    Z = (num2 * num4, (0.0 - num) * num4, num3)
    return X, Y, Z


def world_surface_position(bd, phys, lat, lon, alt):
    """CelestialBody.GetWorldSurfacePosition(lat, lon, alt): bd = BODIES entry, phys = {"pos": [x,y,z], "frame": [A,B,C]}"""
    la, lo = lat * (math.pi / 180.0), lon * (math.pi / 180.0)
    sx, sy, sz = math.cos(la) * math.cos(lo), math.cos(la) * math.sin(lo), math.sin(la)
    nvec = (sx, sz, sy)                       # GetRelSurfaceNVector = SphericalVector(...).xzy
    rel = tuple(c * (bd["radius"] + alt) for c in nvec)   # GetRelSurfacePosition
    v = (rel[0], rel[2], rel[1])              # .xzy
    X, Y, Z = celestial_frame(*phys["frame"])
    w = tuple(v[0] * X[i] + v[1] * Y[i] + v[2] * Z[i] for i in range(3))   # LocalToWorld
    w = (w[0], w[2], w[1])                    # .xzy
    return tuple(w[i] + phys["pos"][i] for i in range(3))


def v3_to_f32(v):
    return tuple(f32(c) for c in v)


def vnorm(a):
    m = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
    if m > 0.0:   # Vector3d.Normalize: num > 0.0
        return (a[0] / m, a[1] / m, a[2] / m)
    return (0.0, 0.0, 0.0)


def vsub(a, c):
    return (a[0] - c[0], a[1] - c[1], a[2] - c[2])


def vdist(a, c):
    d = vsub(a, c)
    return math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])


def gc_positions(R, bpos, p1, p2):
    """CelestialUtilities.GreatCircleDistance(body, position1, position2)"""
    a, c = vnorm(vsub(p1, bpos)), vnorm(vsub(p2, bpos))
    d = a[0] * c[0] + a[1] * c[1] + a[2] * c[2]
    if d > 1.0 or d < -1.0:
        return float("nan")   # Math.Acos outside [-1, 1] (CelestialUtilities.GreatCircleDistance has no clamp)
    return R * math.acos(d)


# ---- a ConfigNode with REFERENCE semantics (AddNode adds the same object; SetNode clears the target and copies into it), for the proto module config
# manipulations of ProtoPartModuleSnapshot.cs (ADJUSTERS / ADJUSTERSTOREMOVE) ------------------------------------------------------------------
class CfgR:
    def __init__(self, name="", values=None, nodes=None):
        self.name = name
        self.values = list(values or [])     # [(k, v)]
        self.nodes = list(nodes or [])       # [CfgR] (shared objects)

    @staticmethod
    def from_cfg(c):
        return CfgR(c.name, list(c.values), [CfgR.from_cfg(n) for n in c.nodes])

    @staticmethod
    def from_json(j):
        return CfgR(j["name"], [(kv[0], kv[1]) for kv in j["values"]], [CfgR.from_json(n) for n in j["nodes"]])

    def to_json(self):
        return {"name": self.name, "values": [[k, v] for k, v in self.values], "nodes": [n.to_json() for n in self.nodes]}

    def get(self, key):
        for k, v in self.values:
            if k == key:
                return v
        return None

    def get_node(self, name):
        for n in self.nodes:
            if n.name == name:
                return n
        return None

    def add_node_obj(self, n):
        self.nodes.append(n)       # ConfigNode.AddNode(ConfigNode): the same object
        return n

    def add_node(self, name):
        n = CfgR(name)
        self.nodes.append(n)
        return n

    def clear_data(self):
        self.nodes = []
        self.values = []

    def copy_into(self, dest):
        """CopyToRecursive(dest): values appended, child nodes copied recursively as NEW nodes"""
        if dest.name == "":
            dest.name = self.name
        for kv in list(self.values):
            dest.values.append(kv)
        for n in list(self.nodes):
            c = dest.add_node(n.name)
            n.copy_into(c)

    def set_node(self, name, new, create=True):
        """ConfigNode.SetNode(name, newNode, createIfNotFound): found -> ClearData + newNode.CopyTo(found) (aliasing included), else the node is added"""
        for n in self.nodes:
            if n.name == name:
                n.clear_data()
                new.copy_into(n)
                return True
        if create:
            new.name = name
            self.nodes.append(new)
            return True
        return False

    def dump(self):
        s = self.name + "{" + "".join("%s=%s;" % kv for kv in self.values)
        for n in self.nodes:
            s += n.dump()
        return s + "}"
