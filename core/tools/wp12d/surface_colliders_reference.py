"""WP-12d independent reference: PQS_Collider.cs, PQS_ColliderAdvanced.cs, PQS_PartCollider.cs written again from the C#, with no
kspcore code. The numerics come from tools/wp12/pqs_ref_math.py (the Mono float rule helpers for managed Unity code, KSP's Vector3d /
QuaternionD, the native Quaternion.FromToRotation checked at start-up against the in-game C3 capture, System.Math Sin / Cos as the
x87 model checked against the in-game libm capture). Terrain heights are WP-11's PQS.GetSurfaceHeight of the stock sphere in its
initial state (no quad built), answered by the WP-12a height oracle (tools/wp12/height_oracle.cpp: WP-11 entry points only; WP-11 is
gated bit for bit by harness/captures/out2/terrain_height.json); the answers are cached in tools/wp12d/height_oracle.json.
radiusMax of each sphere is the capture's `bodies` table.

The scenarios are rigid bodies (owners) moving at constant velocity near the stock KSC, over hills and on the Mun; each step runs
every collider's FixedUpdate in object order, then the harness rule pos = (float)(pos + vel * dt) per component. Recorded per step
and object: the activation flags, every box (localPosition / position, rotation, localScale as float bits, parent, activeSelf) and
the owner's position / velocity after the update; per scenario the Physics.IgnoreCollision calls by collider name.

  python -I tools/wp12d/surface_colliders_reference.py [--oracle-exe build/RelWithDebInfo/tests/wp12_height_oracle.exe]
      -> tests/data/wp12d_colliders_reference.json (missing heights are asked from the oracle exe; without it the cached
         answers must cover every query)
"""
import argparse
import hashlib
import json
import math
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(CORE, "tools", "wp12"))
import pqs_ref_math as M  # noqa: E402

f32 = M.f32
ORACLE_FILE = os.path.join(HERE, "height_oracle.json")
OUT = os.path.join(CORE, "tests", "data", "wp12d_colliders_reference.json")
CAPTURE = os.path.join(M.ROOT, "harness", "captures", "out2", "terrain_height.json")
UP = (0.0, 1.0, 0.0)


def fh(x):
    return "%08x" % M.fbits(x)


def dh(x):
    return "%016x" % M.dbits(x)


def dvec(v):
    return [dh(c) for c in v]


def fvec(v):
    return [fh(c) for c in v]


# ---- managed float helpers not in pqs_ref_math -----------------------------------------------------------------------------
def q_mul_v3(q, p):
    """UnityEngine Quaternion.cs:181 operator*(Quaternion, Vector3) under the Mono rule (locals are float stores)"""
    x, y, z, w = q
    num = f32(x * 2.0)
    num2 = f32(y * 2.0)
    num3 = f32(z * 2.0)
    num4 = f32(x * num)
    num5 = f32(y * num2)
    num6 = f32(z * num3)
    num7 = f32(x * num2)
    num8 = f32(x * num3)
    num9 = f32(y * num3)
    num10 = f32(w * num)
    num11 = f32(w * num2)
    num12 = f32(w * num3)
    return (f32((1.0 - (num5 + num6)) * p[0] + (num7 - num12) * p[1] + (num8 + num11) * p[2]),
            f32((num7 + num12) * p[0] + (1.0 - (num4 + num6)) * p[1] + (num9 - num10) * p[2]),
            f32((num8 - num11) * p[0] + (num9 + num10) * p[1] + (1.0 - (num4 + num5)) * p[2]))


def qd_angle_axis(angle, axis):
    """QuaternionD.cs:151 AngleAxis (degrees; System.Math Cos / Sin = the x87 model)"""
    mag = M.dmag(axis)
    if mag > 0.0001:
        num = M.Cos(angle * (math.pi / 180.0) / 2.0)
        num2 = M.Sin(angle * (math.pi / 180.0) / 2.0)
        return (axis[0] / mag * num2, axis[1] / mag * num2, axis[2] / mag * num2, num)
    return (0.0, 0.0, 0.0, 1.0)


def set_up(box, value):
    """Transform.up setter (UnityEngine Transform.cs:109): rotation = Quaternion.FromToRotation(Vector3.up, value)"""
    box["rotation"] = M.native_from_to_rotation(UP, value)


def new_box(name, scale):
    return {"name": name, "localScale": scale, "parent": "Root", "localPosition": (0.0, 0.0, 0.0), "position": (0.0, 0.0, 0.0),
            "rotation": (0.0, 0.0, 0.0, 1.0), "activeSelf": True}


# ---- the oracle ---------------------------------------------------------------------------------------------------------------
class Oracle:
    def __init__(self):
        self.data = {"bodies": {}}
        if os.path.exists(ORACLE_FILE):
            self.data = json.load(open(ORACLE_FILE, encoding="utf-8"))
        self.pending = {}
        self.used = {}

    def height(self, body, v):
        key = ",".join(dvec(v))
        b = self.data["bodies"].setdefault(body, {})
        self.used.setdefault(body, set()).add(key)
        if key in b:
            return M.from_dhex(b[key])
        self.pending.setdefault(body, {})[key] = dvec(v)
        return None

    def prune(self):
        """keep only the answers the final pass used (the cache is then the same whatever was asked on the way)"""
        pruned = {b: {k: v for k, v in e.items() if k in self.used.get(b, ())} for b, e in self.data["bodies"].items()}
        pruned = {b: e for b, e in pruned.items() if e}
        if pruned != self.data["bodies"]:
            self.data["bodies"] = pruned
            json.dump(self.data, open(ORACLE_FILE, "w", encoding="utf-8", newline="\n"), indent=0, sort_keys=True)

    def resolve(self, exe):
        if not self.pending:
            return False
        if not exe:
            raise SystemExit("height oracle answers missing from %s (%d queries); rerun with --oracle-exe" % (
                ORACLE_FILE, sum(len(v) for v in self.pending.values())))
        q = {"bodies": {b: {"public": [[vec, "initial"] for vec in v.values()], "build": []} for b, v in self.pending.items()}}
        qpath = os.path.join(HERE, "_oracle_queries.json")
        rpath = os.path.join(HERE, "_oracle_results.json")
        json.dump(q, open(qpath, "w", encoding="utf-8"))
        subprocess.run([os.path.abspath(exe), qpath, rpath], check=True)
        res = json.load(open(rpath, encoding="utf-8"))
        os.remove(qpath)
        os.remove(rpath)
        for bname, r in res["bodies"].items():
            b = self.data["bodies"].setdefault(bname, {})
            for key, h in zip(self.pending[bname].keys(), r["public"]):
                b[key] = h
        self.data["provenance"] = {"tool": "core/tools/wp12/height_oracle.cpp (public queries, state initial)", "api": res.get("provenance", {}).get("api")}
        self.pending = {}
        json.dump(self.data, open(ORACLE_FILE, "w", encoding="utf-8", newline="\n"), indent=0, sort_keys=True)
        return True


class Sphere:
    """the PQS members the colliders read: GetSurfaceHeight (oracle), radiusMax, an identity transform"""

    def __init__(self, oracle, body, radius_max):
        self.oracle, self.body, self.radiusMax = oracle, body, radius_max
        self.missing = 0

    def height(self, v):
        h = self.oracle.height(self.body, v)
        if h is None:
            self.missing += 1
            return 0.0
        return h

    # PQS.cs:3042 / :3047 / :3052 with transformPosition 0 and transformRotation identity
    @staticmethod
    def rel_pos(world):
        return world

    @staticmethod
    def rel_dir(world):
        return world

    @staticmethod
    def world_pos(local):
        return local


class IgnoreLog:
    def __init__(self):
        self.calls = []

    def record(self, a, b, ignore=True):
        self.calls.append([a, b, ignore])


# ---- PQS_Collider.cs --------------------------------------------------------------------------------------------------------
class PQSCollider:
    def __init__(self, name, sphere, owner):
        self.name, self.sphere, self.owner = name, sphere, owner
        self.colliderDepth, self.colliderMaxDistance = 100.0, 10.0   # Reset :19
        self.isColliderActive = False

    def start(self):   # :25
        self.colliderObject = new_box(self.owner["name"] + "PQSCollider", (1.0, self.colliderDepth, 1.0))
        self.halfDepth = f32(self.colliderDepth / 2.0)
        self.colliderObject["activeSelf"] = False
        self.isColliderActive = False
        self.set_target_sphere(self.sphere)

    def set_target_sphere(self, s):   # :44
        self.sphere = s
        if s is None:
            self.colliderObject["parent"] = "Owner"
            self.colliderObject["localPosition"] = (0.0, 0.0, 0.0)
            self.deactivate()
        else:
            self.colliderObject["parent"] = "Sphere"
            self.update()

    def fixed_update(self):   # :60
        self.update()

    def update(self):   # :65
        if self.sphere is None:
            return
        vector = M.d2f(self.sphere.rel_pos(self.owner["pos"]))
        normalized = M.v3normalized(vector)
        magnitude = M.v3mag(vector)
        num = f32(self.sphere.height(vector))
        if magnitude - num < self.colliderMaxDistance:
            self.activate()
            self.colliderObject["localPosition"] = M.v3mul(normalized, f32(num - self.halfDepth))
            set_up(self.colliderObject, normalized)
        else:
            self.deactivate()

    def activate(self):   # :86
        if not self.isColliderActive:
            self.colliderObject["activeSelf"] = True
            self.isColliderActive = True

    def deactivate(self):   # :95
        if self.isColliderActive:
            self.colliderObject["activeSelf"] = False
            self.isColliderActive = False

    def boxes(self):
        return [self.colliderObject]

    def flags(self):
        return [self.isColliderActive]


# ---- PQS_ColliderAdvanced.cs ------------------------------------------------------------------------------------------------
ADV_LIST = []
ADV_ROT = []


class PQSColliderAdvanced:
    maxVelocityIteration, velocityThinkAhead, halfColliderHeight, halfCliffHeight, sampleAngle = 4, 2.0, 100.0, 500.0, 0.0001

    def __init__(self, name, sphere, owner, log, grav=True, vel=True):
        self.name, self.sphere, self.owner, self.log = name, sphere, owner, log
        self.colliderSize = (1.0, 1.0)
        self.useGravityCollider, self.useVelocityCollider = grav, vel
        self.isGravityColliderActive = self.isVelocityColliderActive = False

    def own(self):
        return self.owner["name"] + ".own"

    def register(self):   # :91
        for c in ADV_LIST:
            if self.useGravityCollider:
                self.log.record(c.own(), self.grav["name"])
                if c.useGravityCollider:
                    self.log.record(c.grav["name"], self.grav["name"])
            if self.useVelocityCollider:
                self.log.record(c.own(), self.vel["name"])
                self.log.record(c.own(), self.cliff["name"])
                if c.useVelocityCollider:
                    self.log.record(c.vel["name"], self.vel["name"])
                    self.log.record(c.vel["name"], self.cliff["name"])
                    self.log.record(c.cliff["name"], self.vel["name"])
                    self.log.record(c.cliff["name"], self.cliff["name"])
        if self.useVelocityCollider:
            self.log.record(self.vel["name"], self.cliff["name"])
            self.log.record(self.cliff["name"], self.vel["name"])
            if self.useGravityCollider:
                self.log.record(self.vel["name"], self.grav["name"])
                self.log.record(self.cliff["name"], self.grav["name"])
        ADV_LIST.append(self)

    def act_grav(self):   # :138
        if not self.isGravityColliderActive:
            self.isGravityColliderActive = True
            self.log.record(self.own(), self.grav["name"], False)

    def deact_grav(self):   # :147
        if self.isGravityColliderActive:
            self.isGravityColliderActive = False
            self.log.record(self.own(), self.grav["name"], True)

    def act_vel(self):   # :156
        if not self.isVelocityColliderActive:
            self.isVelocityColliderActive = True
            self.log.record(self.own(), self.vel["name"], False)
            self.log.record(self.own(), self.cliff["name"], False)

    def deact_vel(self):   # :166
        if self.isVelocityColliderActive:
            self.isVelocityColliderActive = False
            self.log.record(self.own(), self.vel["name"], True)
            self.log.record(self.own(), self.cliff["name"], True)

    def start(self):   # :176
        if not ADV_ROT:
            ADV_ROT.extend([qd_angle_axis(self.sampleAngle, (1.0, 0.0, 0.0)), qd_angle_axis(self.sampleAngle, (-1.0, 0.0, 0.0)),
                            qd_angle_axis(self.sampleAngle, (0.0, 0.0, 1.0))])
        if self.useGravityCollider:
            self.grav = new_box(self.owner["name"] + "ColGrav", (self.colliderSize[0], f32(self.halfColliderHeight * 2.0), self.colliderSize[1]))
            self.isGravityColliderActive = True
        if self.useVelocityCollider:
            self.vel = new_box(self.owner["name"] + "ColVel", (self.colliderSize[0], f32(self.halfColliderHeight * 2.0), self.colliderSize[1]))
            self.cliff = new_box(self.owner["name"] + "ColVelCliff", (self.colliderSize[0], f32(self.halfCliffHeight * 2.0), self.colliderSize[1]))
            self.isVelocityColliderActive = True
        self.register()
        self.samplesD = [(0.0, 0.0, 0.0)] * 3
        self.set_target_sphere(self.sphere)

    def set_target_sphere(self, s):   # :274
        self.sphere = s
        if s is None:
            if self.useGravityCollider:
                self.grav["parent"], self.grav["localPosition"] = "Owner", (0.0, 0.0, 0.0)
                self.deact_grav()
            if self.useVelocityCollider:
                self.vel["parent"], self.vel["localPosition"] = "Owner", (0.0, 0.0, 0.0)
                self.cliff["parent"], self.cliff["localPosition"] = "Owner", (0.0, 0.0, 0.0)
                self.deact_vel()
        else:
            if self.useGravityCollider:
                self.grav["parent"], self.grav["localPosition"] = "Sphere", (0.0, 0.0, 0.0)
            if self.useVelocityCollider:
                self.vel["parent"], self.vel["localPosition"] = "Sphere", (0.0, 0.0, 0.0)
                self.cliff["parent"], self.cliff["localPosition"] = "Sphere", (0.0, 0.0, 0.0)
            self.update()

    def fixed_update(self):   # :312
        self.update()

    def update(self):   # :317
        if self.sphere is None:
            return
        self.relPos = M.d2f(self.sphere.rel_pos(self.owner["pos"]))
        self.relVel = M.d2f(self.sphere.rel_dir(self.owner["vel"]))
        if self.useGravityCollider:
            self.update_grav()
        if self.useVelocityCollider:
            self.update_vel()

    def update_grav(self):   # :334
        s = self.sphere
        relDir = M.v3normalized(self.relPos)
        radDist = M.v3mag(self.relPos)
        surfaceHeight = s.height(self.relPos)
        altitude = radDist - s.height(self.relPos)
        if altitude < -0.25:
            self.act_grav()
            self.vertical(self.grav, surfaceHeight - self.halfColliderHeight, relDir)
            self.owner["pos"] = M.d2f(M.dmul(relDir, surfaceHeight + 1.0))
            self.owner["vel"] = M.v3mul(self.owner["vel"], f32(0.8))
            return
        radSpeed = M.v3mag(M.v3add(self.relPos, self.relVel)) - radDist
        if radDist > s.radiusMax and radDist + radSpeed > s.radiusMax:
            self.deact_grav()
            return
        planeVelocity = q_mul_v3(M.native_from_to_rotation(M.d2f(relDir), UP), self.relVel)
        verticalSpeed = planeVelocity[1]
        if verticalSpeed < 100.0 and altitude + verticalSpeed < s.radiusMax:
            self.act_grav()
            self.normal_col(self.grav, self.halfColliderHeight, relDir)
        else:
            self.deact_grav()

    def update_vel(self):   # :368
        vel = M.v3normalized(self.relVel)
        speed = M.v3mag(self.relVel) * self.velocityThinkAhead   # conv.r8 of the float product: no float rounding (Mono)
        self.hitPoint = (0.0, 0.0, 0.0)
        self.surfHeight = 0.0
        if speed > 0.25 and self.iterate(self.relPos, vel, speed, 0, self.maxVelocityIteration):
            self.act_vel()
            self.hitPoint = M.dnormalized(self.hitPoint)
            self.normal_col(self.vel, self.halfColliderHeight, self.hitPoint)
            self.vertical(self.cliff, f32(self.surfHeight) - self.halfCliffHeight - 1.0, self.hitPoint)
        else:
            self.deact_vel()

    def iterate(self, start, vel, length, itr, max_itr):   # :388
        self.hitPoint = M.dadd(start, M.dmul(vel, length))
        self.surfHeight = self.sphere.height(self.hitPoint)
        if M.dmag(self.hitPoint) <= self.surfHeight + 0.10000000149011612:
            if itr < max_itr:
                return self.iterate(start, vel, length / 2.0, itr + 1, max_itr)
            return True
        return False

    def vertical(self, col, sh, rn):   # :403
        col["localPosition"] = M.d2f(M.dmul(rn, sh))
        set_up(col, M.d2f(rn))

    def normal_col(self, col, depth, rn):   # :409
        n = self.normal_at(rn)
        col["localPosition"] = M.d2f(M.dsub(M.dmul(rn, self.sphere.height(rn)), M.dmul(n, depth)))
        set_up(col, M.d2f(n))

    def normal_at(self, radian):   # :432
        for i in range(3):
            p = M.qd_mul_v(ADV_ROT[i], radian)
            self.samplesD[i] = M.dmul(p, self.sphere.height(p))
        a = M.dsub(self.samplesD[1], self.samplesD[0])
        c = M.dsub(self.samplesD[2], self.samplesD[0])
        if radian[1] >= 0.0:
            return M.dnormalized((a[1] * c[2] - a[2] * c[1], a[2] * c[0] - a[0] * c[2], a[0] * c[1] - a[1] * c[0]))
        return M.dnormalized((a[2] * c[1] - a[1] * c[2], a[0] * c[2] - a[2] * c[0], a[1] * c[0] - a[0] * c[1]))

    def boxes(self):
        return ([self.grav] if self.useGravityCollider else []) + ([self.vel, self.cliff] if self.useVelocityCollider else [])

    def flags(self):
        return [self.isGravityColliderActive, self.isVelocityColliderActive]


# ---- PQS_PartCollider.cs ----------------------------------------------------------------------------------------------------
PART_LIST = []
PART_ROT = []
PART_STATIC = {"edgeABd": (0.0, 0.0, 0.0), "edgeACd": (0.0, 0.0, 0.0), "normal": (0.0, 0.0, 0.0)}
SAFE_SPOT = (3.4028234663852886e38,) * 3


class PQSPartCollider:
    maxVelocityIteration, velocityThinkAhead, halfColliderHeight, halfCliffHeight, sampleAngle = 2, 2.0, 100.0, 500.0, 0.0001

    def __init__(self, name, sphere, owner, log, grav=True, vel=True, layer=0):
        self.name, self.sphere, self.owner, self.log = name, sphere, owner, log
        self.colliderSize = (1.0, 1.0)
        self.useGravityCollider, self.useVelocityCollider, self.layerID = grav, vel, layer
        self.isGravityColliderActive = self.isVelocityColliderActive = False
        self.colliders = None

    def awake(self):   # :228
        if not PART_ROT:
            PART_ROT.extend([qd_angle_axis(self.sampleAngle, (1.0, 0.0, 0.0)), qd_angle_axis(self.sampleAngle, (-1.0, 0.0, 0.0)),
                             qd_angle_axis(self.sampleAngle, (0.0, 0.0, 1.0))])
        self.samplesD = [(0.0, 0.0, 0.0)] * 3
        self.colliders = None

    def register(self):   # :102
        for c in PART_LIST:
            if self.useGravityCollider:
                for cc in c.colliders:
                    self.log.record(cc, self.grav["name"])
                if c.useGravityCollider:
                    self.log.record(c.grav["name"], self.grav["name"])
            if self.useVelocityCollider:
                for cc in c.colliders:
                    self.log.record(cc, self.vel["name"])
                    self.log.record(cc, self.cliff["name"])
                if c.useVelocityCollider:
                    self.log.record(c.vel["name"], self.vel["name"])
                    self.log.record(c.vel["name"], self.cliff["name"])
                    self.log.record(c.cliff["name"], self.vel["name"])
                    self.log.record(c.cliff["name"], self.cliff["name"])
        if self.useVelocityCollider:
            self.log.record(self.vel["name"], self.cliff["name"])
            self.log.record(self.cliff["name"], self.vel["name"])
            if self.useGravityCollider:
                self.log.record(self.vel["name"], self.grav["name"])
                self.log.record(self.cliff["name"], self.grav["name"])
        PART_LIST.append(self)

    def act_grav(self):   # :158
        if self.isGravityColliderActive:
            return
        self.isGravityColliderActive = True
        for c in self.colliders:
            self.log.record(c, self.grav["name"], False)

    def deact_grav(self):   # :175
        if not self.isGravityColliderActive:
            return
        self.isGravityColliderActive = False
        for c in self.colliders:
            self.log.record(c, self.grav["name"], True)

    def act_vel(self):   # :192
        if self.isVelocityColliderActive:
            return
        self.isVelocityColliderActive = True
        for c in self.colliders:
            self.log.record(c, self.vel["name"], False)
            self.log.record(c, self.cliff["name"], False)

    def deact_vel(self):   # :210
        if not self.isVelocityColliderActive:
            return
        self.isVelocityColliderActive = False
        for c in self.colliders:
            self.log.record(c, self.vel["name"], True)
            self.log.record(c, self.cliff["name"], True)

    def start(self):   # :243 (sic: the cliff box is halfColliderHeight * 2 tall)
        h = f32(self.halfColliderHeight * 2.0)
        if self.useGravityCollider:
            self.grav = new_box(self.owner["name"] + "ColGrav", (self.colliderSize[0], h, self.colliderSize[1]))
            self.isGravityColliderActive = True
        if self.useVelocityCollider:
            self.vel = new_box(self.owner["name"] + "ColVel", (self.colliderSize[0], h, self.colliderSize[1]))
            self.cliff = new_box(self.owner["name"] + "ColVelCliff", (self.colliderSize[0], h, self.colliderSize[1]))
            self.isVelocityColliderActive = True
        self.colliders = list(self.owner["colliders"])
        self.register()
        self.set_target_sphere(self.sphere)

    def set_target_sphere(self, s):   # :291
        self.sphere = s
        if self.colliders is None:
            return
        if s is None:
            if self.useGravityCollider:
                self.grav["position"] = SAFE_SPOT
                self.deact_grav()
            if self.useVelocityCollider:
                self.vel["position"] = SAFE_SPOT
                self.cliff["position"] = SAFE_SPOT
                self.deact_vel()
        else:
            if self.useGravityCollider:
                self.grav["position"] = SAFE_SPOT
            if self.useVelocityCollider:
                self.vel["position"] = SAFE_SPOT
                self.cliff["position"] = SAFE_SPOT

    def fixed_update(self):   # :326 (part == null: a non-kinematic rigid body)
        self.update()

    def update(self):   # :349
        if self.sphere is None:
            return
        self.relPos = self.sphere.rel_pos(self.owner["pos"])
        self.relVel = self.sphere.rel_dir(self.owner["vel"])
        if self.useGravityCollider:
            self.update_grav()
        if self.useVelocityCollider:
            self.update_vel()

    def update_grav(self):   # :366
        s = self.sphere
        relDir = M.dnormalized(self.relPos)
        radDist = M.dmag(self.relPos)
        surfaceHeight = s.height(self.relPos)
        altitude = radDist - surfaceHeight
        if altitude < -0.25:
            self.act_grav()
            self.vertical(self.grav, surfaceHeight - self.halfColliderHeight, relDir)
            self.owner["pos"] = M.d2f(s.world_pos(M.dmul(relDir, surfaceHeight + 1.0)))
            self.owner["vel"] = M.v3mul(self.owner["vel"], f32(0.8))
            return
        radSpeed = M.dmag(M.dadd(self.relPos, self.relVel)) - radDist
        if radDist > s.radiusMax and radDist + radSpeed > s.radiusMax:
            self.deact_grav()
            return
        q = M.native_from_to_rotation(M.d2f(relDir), UP)
        planeVelocity = q_mul_v3(q, M.d2f(self.relVel))
        verticalSpeed = planeVelocity[1]
        if verticalSpeed < 0.0 and altitude + verticalSpeed < s.radiusMax:
            self.act_grav()
            self.normal_col(self.grav, self.halfColliderHeight, relDir)
        else:
            self.deact_grav()

    def update_vel(self):   # :400
        self.relVelNorm = M.dnormalized(self.relVel)
        self.speed = M.dmag(self.relVel) * self.velocityThinkAhead
        self.hitPoint = (0.0, 0.0, 0.0)
        self.surfHeight = 0.0
        self.itr = 0
        if self.speed > 0.25 and self.iterate():
            self.act_vel()
            self.hitPoint = M.dnormalized(self.hitPoint)
            self.normal_col(self.vel, self.halfColliderHeight, self.hitPoint)
            self.vertical(self.cliff, self.surfHeight - self.halfCliffHeight - 1.0, self.hitPoint)
        else:
            self.deact_vel()

    def iterate(self):   # :420
        self.hitPoint = M.dadd(self.relPos, M.dmul(self.relVelNorm, self.speed))
        self.surfHeight = self.sphere.height(self.hitPoint)
        if M.dmag(self.hitPoint) <= self.surfHeight + 0.1:
            if self.itr < self.maxVelocityIteration:
                self.itr += 1
                self.speed *= 0.5
                return self.iterate()
            return True
        return False

    def vertical(self, col, sh, rn):   # :437
        col["position"] = M.d2f(self.sphere.world_pos(M.dmul(rn, sh)))
        set_up(col, M.d2f(rn))

    def normal_col(self, col, depth, rn):   # :443
        self.normal_at(rn)
        n = PART_STATIC["normal"]
        col["position"] = M.d2f(self.sphere.world_pos(M.dsub(M.dmul(rn, self.sphere.height(rn)), M.dmul(n, depth))))
        set_up(col, M.d2f(n))

    def normal_at(self, radian):   # :450
        for i in range(3):
            p = M.qd_mul_v(PART_ROT[i], radian)
            self.samplesD[i] = M.dmul(p, self.sphere.height(p))
        a = M.dsub(self.samplesD[1], self.samplesD[0])
        c = M.dsub(self.samplesD[2], self.samplesD[0])
        PART_STATIC["edgeABd"], PART_STATIC["edgeACd"] = a, c
        if radian[1] >= 0.0:
            PART_STATIC["normal"] = M.dnormalized((a[1] * c[2] - a[2] * c[1], a[2] * c[0] - a[0] * c[2], a[0] * c[1] - a[1] * c[0]))
        else:
            PART_STATIC["normal"] = M.dnormalized((a[2] * c[1] - a[1] * c[2], a[0] * c[2] - a[2] * c[0], a[1] * c[0] - a[0] * c[1]))

    def boxes(self):
        return ([self.grav] if self.useGravityCollider else []) + ([self.vel, self.cliff] if self.useVelocityCollider else [])

    def flags(self):
        return [self.isGravityColliderActive, self.isVelocityColliderActive]


# ---- scenarios ------------------------------------------------------------------------------------------------------------------
def dir_latlon(lat, lon):
    """a unit direction (inputs only: any formula will do, the result is recorded as float bits)"""
    la, lo = math.radians(lat), math.radians(lon)
    return (math.cos(la) * math.cos(lo), math.sin(la), math.cos(la) * math.sin(lo))


def tangent_east(d):
    e = (-d[2], 0.0, d[0])
    n = math.sqrt(e[0] * e[0] + e[2] * e[2])
    return (e[0] / n, 0.0, e[2] / n)


SCENARIOS = [
    # name, body, lat, lon, start altitude above the terrain, down speed, east speed, steps, objects, events
    ("kerbin_ksc_descent", "Kerbin", -0.0972, -74.5577, 40.0, 25.0, 30.0, 120, ["advanced", "part", "collider"], {}),
    ("kerbin_hills_skim", "Kerbin", -0.03, -75.2, 10.0, 6.0, 220.0, 120, ["advanced", "part", "collider", "advanced_grav"], {}),
    ("mun_descent", "Mun", 3.0, 30.0, 30.0, 12.0, 6.0, 120, ["advanced", "part", "collider"], {}),
    ("kerbin_retarget", "Kerbin", -0.1, -74.6, 6.0, 4.0, 3.0, 60, ["advanced", "part", "collider"], {20: "null", 30: "sphere"}),
]
DT = 0.02


def run_scenario(oracle, radius_max, sc):
    name, body, lat, lon, alt, vdown, veast, steps, kinds, events = sc
    del ADV_LIST[:]
    del PART_LIST[:]
    sphere = Sphere(oracle, body, radius_max[body])
    d = dir_latlon(lat, lon)
    h0 = sphere.height(d)
    log = IgnoreLog()
    objs = []
    pos0 = M.d2f(M.dmul(d, h0 + alt))
    e = tangent_east(d)
    vel0 = M.d2f((-d[0] * vdown + e[0] * veast, -d[1] * vdown + e[1] * veast, -d[2] * vdown + e[2] * veast))
    inputs = []
    for i, kind in enumerate(kinds):
        oname = "%s%d" % (kind, i)
        # each owner a little apart so the boxes differ
        off = (0.0, float(i) * 3.0, 0.0)
        owner = {"name": oname, "pos": M.d2f(M.dadd(pos0, off)), "vel": vel0, "colliders": [oname + ".c0", oname + ".c1"]}
        inputs.append({"name": oname, "kind": kind, "position": fvec(owner["pos"]), "velocity": fvec(owner["vel"]),
                       "colliders": owner["colliders"]})
        if kind == "collider":
            objs.append(PQSCollider(oname, sphere, owner))
        elif kind == "advanced":
            objs.append(PQSColliderAdvanced(oname, sphere, owner, log))
        elif kind == "advanced_grav":
            objs.append(PQSColliderAdvanced(oname, sphere, owner, log, vel=False))
        else:
            o = PQSPartCollider(oname, sphere, owner, log)
            o.awake()
            objs.append(o)
    for o in objs:
        o.start()
    rows = []
    for k in range(steps):
        ev = events.get(k)
        if ev is not None:
            for o in objs:
                o.set_target_sphere(None if ev == "null" else sphere)
        for o in objs:
            o.fixed_update()
        row = {"step": k, "objects": []}
        for o in objs:
            row["objects"].append({
                "flags": o.flags(),
                "boxes": [{"name": b["name"], "parent": b["parent"], "active": b["activeSelf"], "scale": fvec(b["localScale"]),
                           "localPosition": fvec(b["localPosition"]), "position": fvec(b["position"]), "rotation": fvec(b["rotation"])}
                          for b in o.boxes()],
                "ownerPosition": fvec(o.owner["pos"]), "ownerVelocity": fvec(o.owner["vel"])})
        rows.append(row)
        for o in objs:
            p, v = o.owner["pos"], o.owner["vel"]
            o.owner["pos"] = (f32(p[0] + v[0] * DT), f32(p[1] + v[1] * DT), f32(p[2] + v[2] * DT))
    return {"name": name, "body": body, "dt": dh(DT), "steps": steps, "events": {str(k): v for k, v in events.items()},
            "objects": inputs, "rows": rows, "ignore": log.calls}, sphere.missing


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--oracle-exe", default=None)
    args = ap.parse_args()
    nat = M.check_natives()
    M.check_x87()
    cap = json.load(open(CAPTURE, encoding="utf-8"))
    bt = [t for t in cap["tables"] if t["name"] == "bodies"][0]
    cols = [c["name"] for c in bt["columns"]]
    radius_max = {r[cols.index("body")]: M.from_dhex(r[cols.index("radiusMax_hex")]) for r in bt["rows"]}
    oracle = Oracle()
    for it in range(400):
        out, missing = [], 0
        oracle.used = {}
        for sc in SCENARIOS:
            res, m = run_scenario(oracle, radius_max, sc)
            out.append(res)
            missing += m
        if missing == 0:
            break
        print("pass %d: %d heights missing" % (it, missing))
        oracle.resolve(args.oracle_exe)
    else:
        raise SystemExit("the oracle did not converge")
    oracle.prune()
    doc = {"schema": "kspcore-wp12d-colliders/1",
           "provenance": {"tool": "core/tools/wp12d/surface_colliders_reference.py",
                          "spec": ["PQS_Collider.cs", "PQS_ColliderAdvanced.cs", "PQS_PartCollider.cs", "UnityEngine Quaternion.cs:181", "Transform.cs:103"],
                          "heights": "core/tools/wp12d/height_oracle.json (sha256 %s)" % hashlib.sha256(open(ORACLE_FILE, "rb").read()).hexdigest(),
                          "radiusMax": "harness/captures/out2/terrain_height.json table bodies",
                          "natives_checked": nat},
           "scenarios": out}
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, sort_keys=True, separators=(",", ":"))
        f.write("\n")
    print("wrote", OUT, os.path.getsize(OUT), "bytes,", sum(len(s["rows"]) for s in out), "rows")


if __name__ == "__main__":
    main()
