"""WP-54 slice 2 reference, part 6: the create actions (Expansions/Missions/Actions/ActionCreate{Asteroid,Comet,Flag,Kerbal}.cs).
Gated: Awake defaults (title), GetInfo, GetNodeBodyParameterString, Load -> Save, and for ActionCreateFlag GetProtoVesselNode / Fire against labelled
synthetic builders (the ProtoVessel node builders, the PQS flag, the unique flight id and the Unity native quaternion operations are stand-ins used by
BOTH sides; the natives are gated by WP-07 / WP-11). ActionCreateVessel / ActionCreateLaunchSite and the Asteroid / Comet / Kerbal GetProtoVesselNode
are not gated (see STATUS). No kspcore code."""
import math

from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, f32, fmt, b, stub_from_to, stub_angle_axis, deg_vec
from ref_data import D, rng_pick, VGL, Orb, HOME, BODIES, body, strip_gender, uint_try, POOL_NUM, ORB_POOL
from ref_restrict import PID_DEFAULT, TICK
from ref_tests3 import MK, gen_mk_cfg, parse_bool

CLASSES = ["A", "B", "C", "D", "E", "F", "G", "H", "I"]
CLASS_TAG = {"A": "#autoLOC_184791", "B": "#autoLOC_184793", "C": "#autoLOC_184795", "D": "#autoLOC_184797", "E": "#autoLOC_184799", "F": "#autoLOC_6011124",
             "G": "#autoLOC_6011125", "H": "#autoLOC_6011126", "I": "#autoLOC_6011129"}
ASTEROID_TYPE_TAG = {"Regular": "#autoLOC_8100110", "Glimmeroid": "#autoLOC_8100111"}
FLAG_URL = "Squad/Flags/default"
FLIGHT_ID = 7777
RADIUS_MULT = 3.0


def node_of(cfg, name):
    for n in cfg.nodes:
        if n.name == name:
            return n
    return None


class AM:
    """ActionModule: title, parametersDisplayedInSAP, restartOnSceneLoad, isRunning"""
    NAME = ""
    TITLE = ""
    INFO = ""

    def __init__(self):
        self.title = fmt(self.TITLE)
        self.sap = []
        self.restart = False
        self.running = False

    def load_base(self, cfg):
        t = cfg.get("title")
        if t is not None:
            self.title = t
        self.sap = cfg.get_all("parametersDisplayedInSAP")
        v = cfg.get("restartOnSceneLoad")
        if v is not None:
            self.restart = parse_bool(v)
        v = cfg.get("isRunning")
        if v is not None:
            self.running = parse_bool(v)

    def save_base(self, out):
        out.values += [("name", self.NAME), ("title", self.title)] + [("parametersDisplayedInSAP", s) for s in self.sap]
        out.values += [("restartOnSceneLoad", b(self.restart)), ("isRunning", b(self.running))]

    def info(self):
        return fmt(self.INFO)

    def base_body(self, field):
        text = fmt("#autoLOC_6003083")
        if field["has"]:
            text = field["value"]
        return fmt("#autoLOC_8004190", field["guiName"], text)


def new_vgl(icon, splashed=False):
    """new VesselGroundLocation(node, icon): the home body and a KSPRandom(Environment.TickCount) position around the KSC (the test pins the tick count)"""
    from ref_common import KSPRandom
    vg = VGL()
    vg.icon = icon
    vg.body = HOME
    vg.splashed = splashed
    rnd = KSPRandom(TICK)
    vg.lat = -0.008999999612569809 + rnd.next_double() * (0.15530000627040863 - -0.008999999612569809)
    vg.lon = -74.82420349121094 + rnd.next_double() * (-74.51119995117188 - -74.82420349121094)
    return vg


class SLoc:
    """ParamChoices_VesselSimpleLocation as the create actions build it in Awake"""

    def __init__(self, icon, choice, splashed):
        self.choice = choice
        self.vg = new_vgl(icon, splashed)
        self.orb = Orb(HOME)

    def load(self, cfg):
        v = cfg.get("locationChoice")
        self.choice = v if v in ("landed", "orbit") else "orbit"
        ln = node_of(cfg, "LANDEDDATA")
        if ln is not None:
            self.vg.load(ln)
        on = node_of(cfg, "ORBITDATA")
        if on is not None:
            self.orb.load(on)

    def save(self, out):
        out.values.append(("locationChoice", self.choice))
        ln = Cfg("LANDEDDATA")
        self.vg.save(ln)
        out.nodes.append(ln)
        on = Cfg("ORBITDATA")
        self.orb.save(on)
        out.nodes.append(on)

    def text(self):
        return self.orb.text() if self.choice == "orbit" else self.vg.text()


class Ast:
    def __init__(self):
        self.name, self.seed, self.type, self.cls, self.pid = "Asteroid", 0, "Regular", "C", 0

    def load(self, cfg):
        v = cfg.get("persistentId")
        if v is not None and v.strip().isdigit() and int(v.strip()) < 2 ** 32:  # TryParse failure keeps the Awake value
            self.pid = int(v.strip())
        v = cfg.get("asteroidType")
        if v in ("Regular", "Glimmeroid"):
            self.type = v
        v = cfg.get("asteroidClass")
        if v in CLASSES:
            self.cls = v
        v = cfg.get("asteroidSeed")
        if v is not None and uint_try(v) is not None:
            self.seed = uint_try(v)
        v = cfg.get("asteroidName")
        if v is not None:
            self.name = v

    def save(self, out):
        out.values += [("asteroidName", self.name), ("asteroidSeed", str(self.seed)), ("asteroidClass", self.cls), ("asteroidType", self.type), ("persistentId", str(self.pid))]


class Com:
    def __init__(self):
        self.name, self.seed, self.radius, self.type, self.cls, self.pid = "Comet", 0, 0.0, "", "C", 0

    def load(self, cfg):
        v = cfg.get("persistentId")
        if v is not None and v.strip().isdigit() and int(v.strip()) < 2 ** 32:  # TryParse failure keeps the Awake value
            self.pid = int(v.strip())
        v = cfg.get("cometClass")
        if v in CLASSES:
            self.cls = v
        v = cfg.get("cometType")
        if v is not None:
            self.type = v
        v = cfg.get("cometradius")
        if v is not None:
            self.radius = f32(float(v))
        v = cfg.get("cometSeed")
        if v is not None and uint_try(v) is not None:
            self.seed = uint_try(v)
        v = cfg.get("cometName")
        if v is not None:
            self.name = v

    def save(self, out):
        out.values += [("cometName", self.name), ("cometSeed", str(self.seed)), ("cometradius", "%s" % ("%.9g" % self.radius)), ("cometType", self.type),
                       ("cometClass", self.cls), ("persistentId", str(self.pid))]


def g9s(x):
    from ref_data import F
    return F(x)


class CreateAsteroid(AM):
    NAME = "ActionCreateAsteroid"
    TITLE = "#autoLOC_8000045"
    INFO = "#autoLOC_8004007"

    def __init__(self):
        super().__init__()
        self.ast = Ast()
        self.loc = SLoc("Asteroid", "orbit", True)

    def load(self, cfg):
        self.load_base(cfg)
        self.ast.load(cfg)
        self.loc.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.ast.save(out)
        self.loc.save(out)
        return out

    def body_text(self, f):
        if f.get("fieldType") == "Asteroid":
            return fmt("#autoLOC_8000139", f["guiName"], fmt(ASTEROID_TYPE_TAG[self.ast.type]), fmt(CLASS_TAG[self.ast.cls]))
        if f["name"] == "location":
            return self.loc.text()
        return self.base_body(f)


class CreateComet(AM):
    NAME = "ActionCreateComet"
    TITLE = "#autoLOC_8005445"
    INFO = "#autoLOC_8005446"

    def __init__(self):
        super().__init__()
        self.com = Com()
        self.loc = SLoc("Comet", "orbit", True)

    def load(self, cfg):
        self.load_base(cfg)
        self.com.load(cfg)
        self.loc.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        c = self.com
        out.values += [("cometName", c.name), ("cometSeed", str(c.seed)), ("cometradius", g9s(c.radius)), ("cometType", c.type), ("cometClass", c.cls),
                       ("persistentId", str(c.pid))]
        self.loc.save(out)
        return out

    def body_text(self, f):
        if f.get("fieldType") == "Comet":
            return fmt("#autoLOC_8000139", f["guiName"], "", fmt(CLASS_TAG[self.com.cls]))  # cometDisplayType stays "" (no comet definitions)
        if f["name"] == "location":
            return self.loc.text()
        return self.base_body(f)


class CreateKerbal(AM):
    NAME = "ActionCreateKerbal"
    TITLE = "#autoLOC_8000240"
    INFO = "#autoLOC_8004008"

    def __init__(self):
        super().__init__()
        self.mk = MK("Crew", True)
        self.loc = SLoc("Kerbal", "landed", True)
        self.pid = PID_DEFAULT
        self.stranded = False
        self.helmet = False
        self.neck = False

    def load(self, cfg):
        self.load_base(cfg)
        self.loc.load(cfg)
        self.mk.load(cfg)
        for k, a in (("isStranded", "stranded"), ("isHelmetEnabled", "helmet"), ("isNeckRingEnabled", "neck")):
            v = cfg.get(k)
            if v is not None:
                setattr(self, a, parse_bool(v))
        v = cfg.get("persistentId")
        if v is not None and v.strip().isdigit() and int(v.strip()) < 2 ** 32:  # TryParse failure keeps the Awake value
            self.pid = int(v.strip())

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.mk.save(out)
        out.values += [("isStranded", b(self.stranded)), ("isHelmetEnabled", b(self.helmet)), ("isNeckRingEnabled", b(self.neck)), ("persistentId", str(self.pid))]
        self.loc.save(out)
        return out

    def body_text(self, f):
        if f["name"] == "location":
            return self.loc.text()
        if f["name"] == "missionKerbal":
            return self.mk.text()
        return self.base_body(f)


def quat_mul(l, r):
    lx, ly, lz, lw = l
    rx, ry, rz, rw = r
    return [f32(lw * rx + lx * rw + ly * rz - lz * ry), f32(lw * ry + ly * rw + lz * rx - lx * rz), f32(lw * rz + lz * rw + lx * ry - ly * rx),
            f32(lw * rw - lx * rx - ly * ry - lz * rz)]


class CreateFlag(AM):
    NAME = "ActionCreateFlag"
    TITLE = "#autoLOC_8004174"
    INFO = "#autoLOC_8004175"

    def __init__(self):
        super().__init__()
        self.site = fmt("#autoLOC_8100159")
        self.plaque = fmt("#autoLOC_8100159")
        self.vg = new_vgl("Flag")
        self.pid = PID_DEFAULT

    def load(self, cfg):
        self.load_base(cfg)
        v = cfg.get("siteName")
        if v is not None:
            self.site = v
        v = cfg.get("plaqueText")
        if v is not None:
            self.plaque = v
        v = cfg.get("persistentId")
        if v is not None and v.strip().isdigit() and int(v.strip()) < 2 ** 32:  # TryParse failure keeps the Awake value
            self.pid = int(v.strip())
        self.vg.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        out.values += [("siteName", self.site), ("plaqueText", self.plaque), ("persistentId", str(self.pid))]
        self.vg.save(out)
        return out

    def body_text(self, f):
        if f["name"] == "location":
            return self.vg.text()
        return self.base_body(f)

    def proto(self, pqs):
        """GetProtoVesselNode against the stand-in builders of the C++ test (see test_missions2.cpp)"""
        if self.site == "":
            self.site = fmt("#autoLOC_8100159")
        bd = body(self.vg.body)
        part = Cfg("PART", [("name", "flag"), ("id", str(FLIGHT_ID))])
        part.values.append(("flag", FLAG_URL))
        mod = Cfg("MODULE", [("name", "FlagSite"), ("state", "Placed"), ("PlaqueText", self.plaque)])
        part.nodes.append(mod)
        v = Cfg("VESSEL", [("name", self.site), ("type", "Flag"), ("orbitRef", bd["name"]), ("orbitSma", D(bd["radius"] * RADIUS_MULT)), ("rootIdx", "0"),
                           ("persistentId", "0"), ("landedAt", ""), ("lat", "0"), ("lon", "0"), ("sit", ""), ("alt", "0"), ("skipGroundPositioning", "True"),
                           ("landed", "False"), ("splashed", "False"), ("vesselSpawning", "False"), ("PQSMin", "-1"), ("PQSMax", "-1")], [part])

        def setv(k, val):
            for i, (kk, _) in enumerate(v.values):
                if kk == k:
                    v.values[i] = (k, val)
                    return
            raise AssertionError(k)

        setv("persistentId", str(self.pid))
        setv("landedAt", bd["name"])
        setv("lat", D(self.vg.lat))
        setv("lon", D(self.vg.lon))
        setv("sit", "LANDED")
        setv("alt", D(self.vg.alt))
        setv("skipGroundPositioning", "False")
        setv("landed", "True")
        setv("splashed", "False")
        v.values.append(("prst", "False"))
        lat = self.vg.lat * (math.pi / 180.0)
        lon = self.vg.lon * (math.pi / 180.0)
        nlat, nlon = math.cos(lat), math.sin(lat)
        sx, sy, sz = nlat * math.cos(lon), nlat * math.sin(lon), nlon  # SphericalVector
        rel = [f32(sx), f32(sz), f32(sy)]  # .xzy
        v.values.append(("nrm", deg_vec([0.0, 1.0, 0.0])))
        angle = f32(self.vg.rot[2] + -30.0)
        q = quat_mul(stub_from_to([0.0, 1.0, 0.0], rel), stub_angle_axis(angle, [0.0, 1.0, 0.0]))
        v.values.append(("rot", deg_vec(q)))
        setv("vesselSpawning", "True")
        if pqs:
            setv("PQSMin", "0")
            setv("PQSMax", "0")
        return v


def head(m):
    return [{"op": "title", "out": m.title}, {"op": "info", "out": m.info()}, {"op": "saved", "out": m.save().dump()}]


def body_ops(m, fields):
    ops = []
    for f in fields:
        f = dict(f)
        f.setdefault("value", "x")
        f.setdefault("has", r_has(f))
        ops.append({"op": "body", "field": f, "out": m.body_text(f)})
    return ops


def r_has(f):
    return True


def loc_cfg(r, cfg, icon_ok=True):
    if r.random() < 0.8:
        cfg.values.append(("locationChoice", rng_pick(r, ["landed", "orbit", "bogus"])))
    ln = Cfg("LANDEDDATA")
    for k in r.sample(["lon", "lat", "alt", "rot", "splashed", "bodyName"], r.randrange(0, 7)):
        if k == "rot":
            ln.values.append((k, rng_pick(r, ["0,0,0", "10,20,30", "90,0,180.5", "270,10,1.25"])))
        elif k == "splashed":
            ln.values.append((k, rng_pick(r, ["True", "False"])))
        elif k == "bodyName":
            ln.values.append((k, rng_pick(r, ["Kerbin", "Mun", "Minmus"])))
        else:
            ln.values.append((k, rng_pick(r, POOL_NUM)))
    if r.random() < 0.8:
        cfg.nodes.append(ln)
    on = Cfg("ORBITDATA")
    for k in r.sample(["SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH"], r.randrange(2, 8)):
        on.values.append((k, rng_pick(r, ORB_POOL[k])))
    on.values.append(("REF", str(r.randrange(0, 3))))
    if r.random() < 0.8:
        cfg.nodes.append(on)


def base_cfg(r, cfg):
    if r.random() < 0.3:
        cfg.values.append(("title", rng_pick(r, ["Custom", "x"])))
    if r.random() < 0.2:
        cfg.values.append(("parametersDisplayedInSAP", "location"))
    if r.random() < 0.3:
        cfg.values.append(("restartOnSceneLoad", rng_pick(r, ["True", "False"])))
    if r.random() < 0.3:
        cfg.values.append(("isRunning", rng_pick(r, ["True", "False"])))


def gen_ast(r):
    m = CreateAsteroid()
    cfg = Cfg("N")
    base_cfg(r, cfg)
    for k in r.sample(["persistentId", "asteroidType", "asteroidClass", "asteroidSeed", "asteroidName"], r.randrange(0, 6)):
        cfg.values.append((k, {"persistentId": rng_pick(r, ["0", "12345", "abc"]), "asteroidType": rng_pick(r, ["Regular", "Glimmeroid"]), "asteroidClass": rng_pick(r, CLASSES),
                               "asteroidSeed": rng_pick(r, ["0", "987654"]), "asteroidName": rng_pick(r, ["Rock 1", "AB-12"])}[k]))
    loc_cfg(r, cfg)
    m.load(cfg)
    ops = head(m) + body_ops(m, [{"name": "asteroid", "guiName": "#autoLOC_8000046", "fieldType": "Asteroid"}, {"name": "location", "guiName": "#autoLOC_8000027"},
                                 {"name": "other", "guiName": "g"}])
    return {"kind": "action", "name": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_comet(r):
    m = CreateComet()
    cfg = Cfg("N")
    base_cfg(r, cfg)
    for k in r.sample(["persistentId", "cometClass", "cometType", "cometradius", "cometSeed", "cometName"], r.randrange(0, 7)):
        cfg.values.append((k, {"persistentId": rng_pick(r, ["0", "777"]), "cometClass": rng_pick(r, CLASSES), "cometType": rng_pick(r, ["Short", "Long"]),
                               "cometradius": rng_pick(r, ["0", "1.5", "250.25"]), "cometSeed": rng_pick(r, ["0", "1633779195", "42"]),
                               "cometName": rng_pick(r, ["Halley", "Churyumov"])}[k]))
    loc_cfg(r, cfg)
    m.load(cfg)
    ops = head(m) + body_ops(m, [{"name": "comet", "guiName": "#autoLOC_6005065", "fieldType": "Comet"}, {"name": "location", "guiName": "#autoLOC_8000027"},
                                 {"name": "other", "guiName": "g", "has": False}])
    return {"kind": "action", "name": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_kerbal(r):
    m = CreateKerbal()
    cfg = Cfg("N")
    base_cfg(r, cfg)
    gen_mk_cfg(r, cfg)
    for k in ("isStranded", "isHelmetEnabled", "isNeckRingEnabled"):
        if r.random() < 0.5:
            cfg.values.append((k, rng_pick(r, ["True", "False"])))
    if r.random() < 0.5:
        cfg.values.append(("persistentId", rng_pick(r, ["0", "555", "bad"])))
    loc_cfg(r, cfg)
    m.load(cfg)
    ops = head(m) + body_ops(m, [{"name": "location", "guiName": "#autoLOC_8000027"}, {"name": "missionKerbal", "guiName": "#autoLOC_8007219"},
                                 {"name": "isStranded", "guiName": "g"}])
    return {"kind": "action", "name": m.NAME, "cfg": cfg.to_json(), "ops": ops, "roster": True}


LATS = ["12.5", "-45.5", "74.5", "90", "-74.5625"]  # the rot component is compared with a tolerance (libm trig), see the C++ test
LONS = ["0.25", "33.25", "-10", "150.75", "90"]


def gen_flag(r):
    m = CreateFlag()
    cfg = Cfg("N")
    base_cfg(r, cfg)
    if r.random() < 0.6:
        cfg.values.append(("siteName", rng_pick(r, ["", "Base One", "Camp"])))
    if r.random() < 0.6:
        cfg.values.append(("plaqueText", rng_pick(r, ["First!", "", "A long\\nplaque"])))
    if r.random() < 0.5:
        cfg.values.append(("persistentId", rng_pick(r, ["0", "321", "x"])))
    exact = r.random() < 0.4
    cfg.values.append(("lat", "0" if exact else rng_pick(r, LATS)))
    cfg.values.append(("lon", "0" if exact else rng_pick(r, LONS)))
    cfg.values.append(("alt", rng_pick(r, ["0", "12.5", "150.75"])))
    cfg.values.append(("rot", rng_pick(r, ["0,0,60", "0,0,90", "10,20,180.5", "0,0,270", "0,0,45.5"])))
    if r.random() < 0.5:
        cfg.values.append(("bodyName", rng_pick(r, ["Kerbin", "Mun", "Minmus"])))
    cfg.values.append(("gizmoIcon", "Flag"))
    m.load(cfg)
    ops = head(m) + body_ops(m, [{"name": "location", "guiName": "#autoLOC_8000179"}, {"name": "siteName", "guiName": "#autoLOC_8004193", "has": False}])
    for pqs in (True, False):
        node = m.proto(pqs)
        ops.append({"op": "proto", "pqs": pqs, "out": node.dump(), "approxRot": not exact})
        ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "action", "name": m.NAME, "cfg": cfg.to_json(), "ops": ops, "flagUrl": FLAG_URL}


def generate(r):
    cases = []
    for _ in range(30):
        cases.append(gen_ast(r))
        cases.append(gen_comet(r))
        cases.append(gen_kerbal(r))
        cases.append(gen_flag(r))
    return cases
