"""WP-54 slice 2 reference, part 4: GoTo, FlyThrough, PlantFlag, StateCrashed, Velocity, Orbit, OrbitParams, CrewAssignment, Grapple, PartDocking, ScienceExperiment,
the Kerbal family (Expansions/Missions/Tests/*.cs). No kspcore code."""
import math

from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, f32, fnum, fmt, tag, b, g9, soi, strip_gender, custom_0hh, custom_0, custom_0h0, nfmt, guid_of
from ref_data import D, F, FS, rng_pick, Area, Volume, Orb, great_circle, body, body_by_name_or_none, HOME, BODIES, mathf_approx, parse_f
from ref_tests2 import (TM, TV, Surf, SitTest, gen_world, gen_vessel, vessel_by_pid, active, vtext, location_ok, margin_ok, gen_surf_cfg, biome_at, convert_tag, pos_near,
                        surf_body_text, UT_POOL, KERBALS, TRAITS, SIT_NAMES)

EPS = 1.401298464324817e-45
ROSTER_CREW = KERBALS
ROSTER_TOURISTS = ["Tourist A", "Tourist B"]


def mathf_clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)


def float_of(x):
    return f32(x)


def parse_bool(v):
    return v.strip().lower() == "true"


# ---- MissionKerbal ----------------------------------------------------------------------------------------------------------------
class MK:
    def __init__(self, type_to_show="Crew", show_any=True, any_text=False):
        self.kerbal = None  # (name, type)
        self.type = type_to_show
        self.show_any = show_any
        self.any_text = any_text

    def any_valid(self):
        return self.kerbal is None

    def is_valid(self, name, typ):
        if self.any_valid():
            return self.show_any and typ == self.type
        return self.kerbal[0] == name and self.kerbal[1] == typ

    def name(self):
        return self.kerbal[0] if self.kerbal else fmt("#autoLOC_8000152")

    def text(self):
        if self.kerbal:
            return fmt("#autoLOC_8002022", self.kerbal[0])
        if self.type == "Tourist":
            return fmt("#autoLOC_8002022", fmt("#autoLOC_8002021"))
        return fmt("#autoLOC_8002022", fmt("#autoLOC_8000152"))

    def save(self, out):
        if self.kerbal:
            out.values.append(("kerbalName", self.kerbal[0]))
        out.values.append(("kerbalType", self.type))

    def load(self, cfg):
        self.kerbal = None
        v = cfg.get("kerbalName")
        if v:
            for n in ROSTER_CREW:
                if n == v:
                    self.kerbal = (n, "Crew")
            for n in ROSTER_TOURISTS:
                if n == v:
                    self.kerbal = (n, "Tourist")
        t = cfg.get("kerbalType")
        self.type = t if t in ("Crew", "Applicant", "Unowned", "Tourist") else "Crew"


def gen_mk_cfg(r, cfg):
    if r.random() < 0.7:
        cfg.values.append(("kerbalName", rng_pick(r, KERBALS + ROSTER_TOURISTS + ["Nobody Kerman"])))
    if r.random() < 0.7:
        cfg.values.append(("kerbalType", rng_pick(r, ["Crew", "Tourist", "Applicant", "Bogus"])))


def crew_of(v):
    out = []
    for p in v["parts"]:
        for n in p["crew"]:
            out.append(n)
    return out


# ---- GoTo --------------------------------------------------------------------------------------------------------------------------
class GoTo(TV):
    NAME = "TestGoTo"
    TITLE = "#autoLOC_8000145"
    INFO = "#autoLOC_8004035"

    def __init__(self):
        super().__init__()
        self.area = Area()
        self.area.radius = 150000.0
        self.speed = 0.10000000149011612
        self.ignore = False
        self.passed = False

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        self.area.load(cfg)
        v = cfg.get("surfaceVelocity")
        if v is not None:
            self.speed = float(v)
        v = cfg.get("ignoreSurfaceVelocity")
        if v is not None:
            self.ignore = parse_bool(v)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        self.area.save(out)
        out.values.append(("surfaceVelocity", D(self.speed)))
        out.values.append(("ignoreSurfaceVelocity", b(self.ignore)))
        return out

    def test(self, w):
        self.tv_test(w)
        v = self.vessel
        if not (self.ignore or (v is not None and v["srf"] <= self.speed + EPS)):
            return False
        if v is not None and v["sit"] in ("LANDED", "SPLASHED"):
            if self.area.dist(f32(v["lat"]), f32(v["lon"])) <= self.area.radius:
                self.passed = True
            return self.passed
        return False


GoTo.body_text = lambda self, f: self.area.text() if f.get("fieldType") == "SurfaceArea" else self.base_body(f)


# ---- FlyThrough --------------------------------------------------------------------------------------------------------------------
class FlyThrough(TV):
    NAME = "TestFlyThrough"
    TITLE = "#autoLOC_8000133"
    INFO = "#autoLOC_8004034"

    def __init__(self):
        super().__init__()
        self.vol = Volume()
        self.show_label = False
        self.show_map = True
        self.passed = False

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        self.vol.load(cfg)
        v = cfg.get("showNodeLabel")
        if v is not None:
            self.show_label = parse_bool(v)
        v = cfg.get("showMapNode")
        if v is not None:
            self.show_map = parse_bool(v)
        self.vol.show = self.show_label

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        self.vol.save(out)
        out.values += [("showNodeLabel", b(self.show_label)), ("showMapNode", b(self.show_map))]
        return out

    def test(self, w):
        self.tv_test(w)
        v = self.vessel
        if v is not None:
            if self.vol.inside(v["lat"], v["lon"], v["alt"]):
                self.passed = True
            return self.passed
        return False


FlyThrough.body_text = lambda self, f: self.vol.text() if f.get("fieldType") == "SurfaceVolume" else self.base_body(f)


# ---- PlantFlag / StateCrashed -------------------------------------------------------------------------------------------------------
class PlantFlag(TM):
    NAME = "TestPlantFlag"
    TITLE = "#autoLOC_8000192"
    INFO = "#autoLOC_8004026"

    def __init__(self):
        super().__init__()
        self.s = Surf(False)
        self.found = False
        self.v = None

    def load(self, cfg):
        self.load_base(cfg)
        self.s.load(cfg)
        v = cfg.get("eventFound")
        if v is not None:
            self.found = parse_bool(v)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.s.save(out)
        out.values.append(("eventFound", b(self.found)))
        return out

    def initialized(self):
        self.found = False

    def planted(self, v):
        self.v = v
        mb = v["mainBody"]
        c = self.s.choice
        if c == "bodyData":
            if self.s.valid_body(mb):
                self.found = True
        elif c == "biomeData":
            if mb == self.s.biome[0]:
                cur = biome_at(mb, v["lat"], v["lon"])
                if self.s.biome[1] == "" or cur.lower() == self.s.biome[1].lower():
                    self.found = True
        elif c == "areaData":
            if mb == self.s.area.body:
                self.found = self.s.area.dist(v["lat"], v["lon"]) <= self.s.area.radius
        else:
            self.found = True


class Crashed(TV):
    NAME = "TestVesselStateCrashed"
    TITLE = "#autoLOC_8000242"
    INFO = "#autoLOC_8004006"

    def __init__(self):
        super().__init__()
        self.s = Surf(True, "anyData")
        self.found = False

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        self.s.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        self.s.save(out)
        return out

    def initialized(self):
        self.found = False

    def destroyed(self, w, v):
        if not self.found:
            self.tv_test(w)
            if self.vessel is v and self.location():
                self.found = True

    def location(self):
        v = self.vessel
        c = self.s.choice
        if c == "anyData":
            return True
        res, _ = location_ok(self.s, v)
        return bool(res)


# ---- Velocity --------------------------------------------------------------------------------------------------------------------
class Velocity(TV):
    NAME = "TestVesselVelocity"
    TITLE = "#autoLOC_8004164"
    INFO = "#autoLOC_8004165"

    def __init__(self):
        super().__init__()
        self.velocity = 0.0
        self.op = "GreaterThan"
        self.frame = "Default"
        self.stype = "SurfaceVelocity"
        self.kerbal = MK("Crew", False)
        self.vid = 0
        self.aid = 0
        self.cid = 0

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        v = cfg.get("velocity")
        if v is not None:
            self.velocity = parse_f(v)
        v = cfg.get("comparisonOperator")
        self.op = v if v in ("LessThan", "GreaterThan") else "GreaterThan"
        v = cfg.get("speedType")
        self.stype = v if v in ("SurfaceVelocity", "OrbitalVelocity") else "SurfaceVelocity"
        v = cfg.get("referenceBody")
        self.frame = v if v in ("Default", "Asteroid", "Comet", "Kerbal", "Vessel") else "Default"
        for k, a in (("velocityVesselID", "vid"), ("velocityAsteroidID", "aid"), ("velocityCometID", "cid")):
            v = cfg.get(k)
            if v is not None:
                setattr(self, a, int(v))
        for n in cfg.nodes:
            if n.name == "VELOCITYWRTKERBAL":
                self.kerbal.load(n)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        out.values += [("velocity", F(self.velocity)), ("comparisonOperator", self.op), ("speedType", self.stype), ("referenceBody", self.frame), ("velocityVesselID", str(self.vid)),
                       ("velocityAsteroidID", str(self.aid)), ("velocityCometID", str(self.cid))]
        k = Cfg("VELOCITYWRTKERBAL")
        self.kerbal.save(k)
        out.nodes.append(k)
        return out

    def test(self, w):
        self.tv_test(w)
        if self.vessel is None:
            return True
        num = 0.0
        ref = None
        if self.frame in ("Default",):
            num = self.vessel["srf"] if self.stype == "SurfaceVelocity" else self.vessel["obt"]
        else:
            if self.frame == "Asteroid":
                ref = vessel_by_pid(w, self.aid)
            elif self.frame == "Comet":
                ref = vessel_by_pid(w, self.cid)
            elif self.frame == "Kerbal":
                for v in w["vessels"]:
                    if any(self.kerbal.is_valid(n, "Crew") for n in crew_of(v)):
                        ref = v
                        break
                if ref is None:
                    ref = getattr(self, "stale", None)  # vesselRefBody is a field: the previous Test() value survives
            else:
                ref = vessel_by_pid(w, self.vid)
            if ref is None and self.frame != "Kerbal":
                raise KeyError("PersistentVesselIds")
            if self.frame != "Default":
                self.stale = ref
            if ref is not None:
                sv = w["shipObtVel"]
                d = [sv[i] - ref["obtVel"][i] for i in range(3)]
                num = ref_mag([f32(c) for c in d])
        if self.op == "GreaterThan" and not (num < self.velocity):
            return True
        if self.op == "LessThan":
            return num <= self.velocity
        return False


def _velocity_body(self, f):
    if f["name"] == "velocity":
        return fmt("#autoLOC_8100154", f["guiName"], "<" if self.op == "LessThan" else ">", custom_0(self.velocity))
    if f["name"] == "referenceBody":
        text = self.base_body(f) + "\n"
        if self.frame == "Asteroid":
            return text + fmt("#autoLOC_8004190", fmt("#autoLOC_8000046"), fmt("#autoLOC_6003000"))
        if self.frame == "Comet":
            return text + fmt("#autoLOC_8004190", fmt("#autoLOC_6005065"), fmt("#autoLOC_6003000"))
        if self.frame == "Kerbal":
            return text + self.kerbal.text()
        return text
    return self.base_body(f)


Velocity.body_text = _velocity_body


def ref_mag(a):
    return f32(math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]))


# ---- Orbit ----------------------------------------------------------------------------------------------------------------------------
def orbit_elements(v):
    return v["orbit"]


def synth_at_orbit(o, dev, v):
    """the synthetic VesselUtilities.VesselAtOrbit stand-in installed on both sides: same reference body and a semi-major axis within dev percent"""
    vo = v["orbit"]
    if vo["ref"] != o["ref"]:
        return False
    return abs(vo["sma"] - o["sma"]) <= dev / 100.0 * abs(o["sma"])


def synth_at_orbit_acc(o, dev, v):
    if not synth_at_orbit(o, dev, v):
        return 0.0
    return 1.0 - abs(v["orbit"]["sma"] - o["sma"]) / abs(o["sma"])


class OrbitT(TV):
    NAME = "TestOrbit"
    TITLE = "#autoLOC_8000158"
    INFO = "#autoLOC_8004023"

    def __init__(self):
        super().__init__()
        self.acc = 90.0
        self.stab = 5.0
        self.thrust = False
        self.orb = Orb(HOME)
        self.rel = None
        self.start = -1.0

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        v = cfg.get("orbitAccuracy")
        if v is not None:
            self.acc = float(v)
        else:
            t = cfg.get("orbitTolerance")
            tol = parse_f(t) if t is not None else 0.1
            self.acc = 100.0 - f32(tol * 100.0)
        v = cfg.get("stabilizationTime")
        if v is not None:
            self.stab = float(v)
        v = cfg.get("underThrust")
        if v is not None:
            self.thrust = parse_bool(v)
        for n in cfg.nodes:
            if n.name == "ORBIT":
                self.orb = Orb.from_cfg(n)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        out.values += [("orbitAccuracy", D(self.acc)), ("stabilizationTime", D(self.stab)), ("underThrust", b(self.thrust))]
        o = Cfg("ORBIT")
        self.orb.save(o)
        out.nodes.append(o)
        return out

    def deviation(self):
        return mathf_clamp(f32(100.0 - f32(self.acc)), 0.0, 100.0)

    def initialized(self):
        o = self.orb
        self.rel = {"ref": o.orb_ref, "sma": o.orb_sma}

    def test(self, w):
        self.tv_test(w)
        v = self.vessel
        ok = v is not None and v["orbit"]["ref"] == self.orb.orb_ref
        if not ok:
            self.start = -1.0
            return False
        if not self.thrust and v["gee"] > 0.10000000149011612:
            self.start = -1.0
            return False
        osucc = False
        if self.rel is not None:
            osucc = synth_at_orbit(self.rel, self.deviation(), v)
        if not osucc:
            self.start = -1.0
            return False
        if self.start < 0.0:
            self.start = w["ut"]
        return self.start + self.stab < w["ut"]

    def body_text(self, field):
        o = self.orb
        if field.get("fieldType") == "MissionOrbit":
            s = fmt("#autoLOC_8100316", field["guiName"], strip_gender(body(o.orb_ref)["display"])) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100058"), nfmt(o.sma, 0)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100059"), nfmt(o.apoapsis(), 0)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100060"), nfmt(o.periapsis(), 0)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100061"), custom_0hh(o.ecc)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100062"), custom_0hh(o.inc) + "°") + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100063"), custom_0hh(o.lan)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100064"), nfmt(o.argpe, 0)) + "\n"
            s += fmt("#autoLOC_8004190", fmt("#autoLOC_8100065"), nfmt(o.deg_mna, 0))
            return s
        if field["name"] == "orbitAccuracy":
            return fmt("#autoLOC_8004190", field["guiName"], custom_0hh(self.acc) + "%")
        return self.base_body(field)


# ---- OrbitParams ------------------------------------------------------------------------------------------------------------------------
def fmod(a, c):
    return math.fmod(a, c)


class OrbitParams(TV):
    NAME = "TestOrbitParams"
    TITLE = "#autoLOC_8003152"
    INFO = "#autoLOC_8003153"

    def __init__(self):
        super().__init__()
        self.cap = self.cpe = self.cinc = self.clan = False
        self.apmin, self.apmax, self.pemin, self.pemax = 75000.0, 80000.0, 70000.0, 75000.0
        self.inc, self.incacc, self.lan, self.lanacc = 0.0, 90.0, 0.0, 90.0
        self.body = HOME  # MissionCelestialBody(): the home body
        self.stab = 5.0
        self.thrust = False
        self.start = -1.0

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        for k, a, t in (("checkApoapsis", "cap", "b"), ("checkPeriapsis", "cpe", "b"), ("checkInclination", "cinc", "b"), ("checkLAN", "clan", "b"), ("apMaxValue", "apmax", "d"),
                        ("apMinValue", "apmin", "d"), ("peMaxValue", "pemax", "d"), ("peMinValue", "pemin", "d"), ("inclination", "inc", "d"), ("inclinationAccuracy", "incacc", "d"),
                        ("LAN", "lan", "d"), ("LANAccuracy", "lanacc", "d"), ("stabilizationTime", "stab", "d"), ("underThrust", "thrust", "b")):
            v = cfg.get(k)
            if v is not None:
                setattr(self, a, parse_bool(v) if t == "b" else float(v))
        bn = cfg.get("bodyName")
        if bn is not None and bn != "":
            self.body = None if bn == "AnyValid" else body_by_name_or_none(bn)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        out.values += [("checkApoapsis", b(self.cap)), ("checkPeriapsis", b(self.cpe)), ("checkInclination", b(self.cinc)), ("checkLAN", b(self.clan)), ("apMaxValue", D(self.apmax)),
                       ("apMinValue", D(self.apmin)), ("peMaxValue", D(self.pemax)), ("peMinValue", D(self.pemin)), ("inclination", D(self.inc)),
                       ("inclinationAccuracy", D(self.incacc)), ("LAN", D(self.lan)), ("LANAccuracy", D(self.lanacc)), ("stabilizationTime", D(self.stab)),
                       ("underThrust", b(self.thrust)), ("bodyName", "AnyValid" if self.body is None else body(self.body)["name"])]
        return out

    def incdev(self):
        return mathf_clamp(f32(100.0 - f32(self.incacc)), 0.0, 100.0)

    def landev(self):
        return mathf_clamp(f32(100.0 - f32(self.lanacc)), 0.0, 100.0)

    @staticmethod
    def orbit_of(v):
        o = v["orbit"]
        R = body(o["ref"])["radius"]
        return {"ApA": (1.0 + o["ecc"]) * o["sma"] - R, "PeA": (1.0 - o["ecc"]) * o["sma"] - R, "inc": o["inc"], "lan": o["lan"]}

    @staticmethod
    def sit_of(v):
        return v["sit"] if v["loaded"] else v["protoSit"]

    @staticmethod
    def within(v1, v2, dev, full):
        """SystemUtilities.WithinDeviationByValue: the deviation is clamped to 0..100 and treated as a percentage when > 1"""
        d = 100.0 if dev > 100.0 else dev
        d = 0.0 if d < 0.0 else d
        d = d / 100.0 if d > 1.0 else d
        return abs(v1 - v2) / full <= d

    def test(self, w):
        self.tv_test(w)
        v = self.vessel
        ok = v is not None
        if v is not None and self.body is not None:
            ok = ok and body(self.body)["name"] == body(v["mainBody"])["name"]
        if not ok:
            self.start = -1.0
            return False
        if not self.thrust and v["gee"] > 0.10000000149011612:
            self.start = -1.0
            return False
        orb = self.orbit_of(v) if v["loaded"] else self.orbit_of_proto(v)
        sit = self.sit_of(v)
        if sit not in ("ORBITING", "ESCAPING", "SUB_ORBITAL"):
            return False
        osucc = True
        if self.cap:
            osucc = osucc and orb["ApA"] >= self.apmin and orb["ApA"] <= self.apmax
        if self.cpe:
            osucc = osucc and orb["PeA"] >= self.pemin and orb["PeA"] <= self.pemax
        if self.cinc:
            osucc = osucc and self.within(abs(orb["inc"]), abs(self.inc), self.incdev(), 90.0)
        if self.clan and osucc and fmod(abs(orb["inc"]), 180.0) > 1.0:
            flag = flag2 = False
            n = orb["lan"]
            n2 = self.lan
            if orb["inc"] < 0.0:
                n = fmod(n + 180.0, 360.0)
            if self.cinc and self.inc < 0.0:
                n2 = fmod(n2 + 180.0, 360.0)
            n3 = f32(fmod(f32(abs(n - n2)), 360.0))
            if n3 > 180.0:
                n3 = f32(360.0 - n3)
            if f32(n3 / 180.0) < self.landev() / 100.0:
                flag = True
            if self.cinc:
                n4 = f32(self.inc - self.incdev() / 100.0 * 90.0)
                if n4 < 0.0:
                    n4 = abs(n4)
                    if abs(orb["inc"]) < n4:
                        n3 = f32(180.0 - n3)
                        if f32(n3 / 180.0) < self.landev() / 100.0:
                            flag2 = True
            osucc = flag or flag2
        if not osucc:
            self.start = -1.0
            return False
        if self.start < 0.0:
            self.start = w["ut"]
        return self.start + self.stab < w["ut"]

    def orbit_of_proto(self, v):
        return self.orbit_of(v)

    def accuracy(self, v):
        orb = self.orbit_of(v)
        num2 = 0
        num3 = 0.0
        if self.cap:
            num2 += 1
            num3 += abs(f32(f32(orb["ApA"]) / f32(f32(f32(self.apmin) + f32(self.apmax)) / 2.0) - 1.0))
        if self.cpe:
            num2 += 1
            num3 += abs(f32(f32(orb["PeA"]) / f32(f32(f32(self.pemin) + f32(self.pemax)) / 2.0) - 1.0))
        if self.cinc:
            num2 += 1
            dev = abs(orb["inc"] - self.inc) / 90.0
            num3 += min(1.0, max(0.0, f32(dev)))
        if self.clan:
            num2 += 1
            if fmod(abs(orb["inc"]), 180.0) > 1.0:
                n4 = orb["lan"]
                n5 = self.lan
                if orb["inc"] < 0.0:
                    n4 = fmod(n4 + 180.0, 360.0)
                if self.cinc and self.inc < 0.0:
                    n5 = fmod(n5 + 180.0, 360.0)
                n6 = f32(fmod(f32(abs(n4 - n5)), 360.0))
                if n6 > 180.0:
                    n6 = f32(360.0 - n6)
                if f32(n6 / 180.0) < self.landev() / 100.0:
                    num3 += min(1.0, max(0.0, f32(n6 / 180.0)))
                if self.cinc:
                    n7 = f32(self.inc - self.incdev() / 100.0 * 90.0)
                    if n7 < 0.0:
                        n7 = abs(n7)
                        if abs(orb["inc"]) < n7:
                            n6 = f32(180.0 - n6)
                            if f32(n6 / 180.0) < self.landev() / 100.0:
                                num3 += min(1.0, max(0.0, f32(n6 / 180.0)))
            else:
                num3 += 1.0
        if num2 == 0:
            return 1.0
        return num3 / num2

    def body_text(self, field):
        n = field["name"]
        if n == "checkApoapsis":
            return fmt("#autoLOC_8002086", nfmt(self.apmin, 0), nfmt(self.apmax, 0)) if self.cap else ""
        if n == "checkPeriapsis":
            return fmt("#autoLOC_8002087", nfmt(self.pemin, 0), nfmt(self.pemax, 0)) if self.cpe else ""
        if n == "checkInclination":
            return fmt("#autoLOC_8002088", custom_0hh(self.incacc), custom_0hh(self.inc)) if self.cinc else ""
        if n == "checkLAN":
            return fmt("#autoLOC_8002089", custom_0hh(self.lanacc), custom_0hh(self.lan)) if self.clan else ""
        if n == "body":
            t = fmt("#autoLOC_8000273") if self.body is None else strip_gender(body(self.body)["display"])
            return fmt("#autoLOC_8100316", field["guiName"], t)
        if n in ("apMinValue", "apMaxValue", "peMinValue", "peMaxValue", "inclination", "LAN", "LANAccuracy", "InclinationAccuracy"):
            return ""
        return self.base_body(field)

    def validation(self):
        out = []
        if self.cap and self.apmin > self.apmax:
            out.append(["warn", fmt("#autoLOC_8002090")])
        if self.cpe and self.pemin > self.pemax:
            out.append(["warn", fmt("#autoLOC_8002091")])
        if not self.cap and not self.cpe and not self.cinc and not self.clan:
            out.append(["fail", fmt("#autoLOC_8002092")])
        return out


# ---- CrewAssignment ------------------------------------------------------------------------------------------------------------------
class Pair:
    def __init__(self):
        self.vid = 0
        self.pid = 0
        self.name = "#autoLOC_8001004"

    def load(self, cfg):
        for k, a, t in (("partPersistentId", "pid", int), ("vesselPersistentId", "vid", int), ("partName", "name", str)):
            v = cfg.get(k)
            if v is not None:
                setattr(self, a, t(v))

    def save(self, out):
        out.values += [("partPersistentId", str(self.pid)), ("vesselPersistentId", str(self.vid)), ("partName", self.name)]


def all_parts(w):
    for v in w["vessels"]:
        for p in v["parts"]:
            yield v, p


class CrewAssign(TM):
    NAME = "TestCrewAssignment"
    TITLE = "#autoLOC_8000070"
    INFO = "#autoLOC_8004011"

    def __init__(self):
        super().__init__()
        self.pair = Pair()
        self.mk = MK("Crew", True)

    def load(self, cfg):
        self.load_base(cfg)
        self.pair.load(cfg)
        self.mk.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.pair.save(out)
        self.mk.save(out)
        return out

    def contains(self, names):
        return any(self.mk.is_valid(n, "Crew") for n in names)

    def test(self, w):
        if self.pair.pid == 0:
            v = vessel_by_pid(w, self.pair.vid)
            if v is not None:
                self.vessel = v
            if self.vessel is None:
                return True
            for p in self.vessel["parts"]:
                if p["cap"] > 0 and self.contains(p["crew"]):
                    return True
            return False
        for v, p in all_parts(w):
            if p["pid"] == self.pair.pid:
                return self.contains(p["crew"])
        return True

    def body_text(self, field):
        if field["name"] == "missionKerbal":
            return self.mk.text()
        if field["name"] == "vesselID":
            return fmt("#autoLOC_8000068")
        if field["name"] == "vesselPartIDs":
            return fmt("#autoLOC_8006061", fmt(self.pair.name))
        return self.base_body(field)

    def part_id_changed(self, vid, old, new):
        if self.pair.vid == vid and self.pair.pid == old:
            self.pair.pid = new

    def vessel_id_changed(self, old, new):
        if self.pair.vid == old:
            self.pair.vid = new

    def undocking(self, old_v, new_v):
        if self.pair.vid != old_v["pid"] and self.pair.vid != new_v["pid"]:
            return
        for p in new_v["parts"]:
            if self.pair.pid == p["pid"]:
                self.pair.vid = new_v["pid"]
                return


# ---- PartDocking ------------------------------------------------------------------------------------------------------------------------
class PartDock(TM):
    NAME = "TestPartDocking"
    TITLE = "#autoLOC_8002013"
    TITLE_RAW = True
    INFO = "#autoLOC_8002016"

    def __init__(self):
        super().__init__()
        self.p1 = Pair()
        self.p2 = Pair()
        self.dock = "Docked"
        self.first = True
        self.success = False

    def load(self, cfg):
        self.load_base(cfg)
        for n in cfg.nodes:
            if n.name == "PARTONE":
                self.p1.load(n)
            elif n.name == "PARTTWO":
                self.p2.load(n)
        self.dock = "Docked"
        v = cfg.get("dockedUndocked")
        if v is not None:
            self.dock = v

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        n1 = Cfg("PARTONE")
        self.p1.save(n1)
        out.nodes.append(n1)
        n2 = Cfg("PARTTWO")
        self.p2.save(n2)
        out.nodes.append(n2)
        out.values.append(("dockedUndocked", self.dock))
        return out

    def initialized(self):
        self.success = False

    @staticmethod
    def find_vessel(w, part_id):
        if part_id == 0:
            return None
        for v, p in all_parts(w):
            if p["pid"] == part_id:
                return v["pid"]
        return None

    def set_roots(self, w):
        if self.p1.pid == 0:
            flag = False
            v = vessel_by_pid(w, self.p1.vid)
            if v is not None and len(v["parts"]) > 0:
                self.p1.pid = v["parts"][0]["pid"]
                flag = True
            if not flag:
                self.success = True
        if self.p2.pid != 0:
            return
        flag2 = False
        v = vessel_by_pid(w, self.p2.vid)
        if v is not None and len(v["parts"]) > 0:
            if v["loaded"]:
                self.p2.pid = v["parts"][0]["pid"]
            else:
                self.p1.pid = v["parts"][0]["pid"]  # sic: the C# writes partOnevesselPartIDs
            flag2 = True
        if not flag2:
            self.success = True

    def parts_changed(self, w):
        if self.p1.pid == 0 or self.p2.pid == 0:
            self.set_roots(w)
        a = self.find_vessel(w, self.p1.pid)
        c = self.find_vessel(w, self.p2.pid)
        if a is None or c is None:
            return
        if self.dock == "Docked":
            if a == c:
                self.success = True
        elif a != c:
            self.success = True

    def test(self, w):
        if self.first:
            self.parts_changed(w)
            self.first = False
        return self.success

    def body_text(self, field):
        n = field["name"]
        if n in ("partOnevesselPartIDs", "partTwovesselPartIDs"):
            one = n == "partOnevesselPartIDs"
            pair = self.p1 if one else self.p2
            tpl = "#autoLOC_8006059" if one else "#autoLOC_8006060"
            name_any = pair.name if one else self.p1.name
            t2 = fmt("#autoLOC_8001004") if (pair.name == "Any" or name_any == "#autoLOC_8001004") else fmt(pair.name)
            return fmt(tpl, t2)
        if n == "dockedUndocked" and field["has"]:
            if field["value"] == "Docked":
                return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8002014"))
            return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8002015"))
        return self.base_body(field)

    def part_id_changed(self, vid, old, new):
        if self.p1.vid == vid and self.p1.pid == old:
            self.p1.pid = new
        if self.p2.vid == vid and self.p2.pid == old:
            self.p2.pid = new

    def vessel_id_changed(self, old, new):
        if self.p1.vid == old:
            self.p1.vid = new
        if self.p2.vid == old:
            self.p2.vid = new

    def vessel_docking(self, w, old, new):
        if (self.p1.vid == old and self.p1.pid == 0) or (self.p2.vid == old and self.p2.pid == 0):
            self.set_roots(w)


# ---- Grapple ----------------------------------------------------------------------------------------------------------------------------
class Grapple(TM):
    NAME = "TestGrapple"
    TITLE = "#autoLOC_8002013"
    TITLE_RAW = True
    INFO = "#autoLOC_8005462"

    def __init__(self):
        super().__init__()
        self.gv = 0
        self.obj = "Asteroid"
        self.aid = 0
        self.cid = 0
        self.vid = 0
        self.dock = None
        self.so_part = 0
        self.grab = None  # the cached part id holding a grapple node
        self.first = True
        self.success = False

    def load(self, cfg):
        self.load_base(cfg)
        for n in cfg.nodes:
            if n.name == "PARTONE":
                p = Pair()
                p.load(n)
                self.gv = p.vid
        self.dock = "Docked"
        v = cfg.get("dockedUndocked")
        if v is not None:
            self.dock = v
        for k, a in (("grapplingVesselID", "gv"), ("grappleAsteroidID", "aid"), ("grappleCometID", "cid"), ("grappleVesselID", "vid")):
            v = cfg.get(k)
            if v is not None:
                setattr(self, a, int(v))
        v = cfg.get("grappledSpaceObject")
        self.obj = v if v in ("Asteroid", "Comet", "Vessel") else "Asteroid"

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        out.values += [("grapplingVesselID", str(self.gv)), ("grappleAsteroidID", str(self.aid)), ("grappleCometID", str(self.cid)), ("grappleVesselID", str(self.vid)),
                       ("grappledSpaceObject", self.obj), ("dockedUndocked", self.dock)]
        return out

    def initialized(self):
        self.success = False

    def parts_changed(self, w):
        if self.obj == "Comet":
            num2 = self.cid
        elif self.obj == "Vessel":
            num2 = self.vid
        else:
            num2 = self.aid
        act = active(w)
        if self.gv == 0:
            if act is None:
                return
            num = act["pid"]
        else:
            num = self.gv
        if vessel_by_pid(w, num) is not None and vessel_by_pid(w, num2) is not None:
            if self.dock == "Docked":
                if num == num2:
                    self.success = True
            elif num != num2:
                self.success = True
        else:
            if self.obj == "Vessel" or act is None:
                return
            i = len(act["parts"]) - 1
            while i >= 0:
                if act["parts"][i].get("ast"):
                    self.so_part = act["parts"][i]["pid"]
                    break
                i -= 1
            if self.grab is None:
                for p in act["parts"]:
                    if p.get("grapple") is not None:
                        self.grab = p  # the module reference is cached for good (stays tied to the world it was found in)
                        break
            if self.so_part != 0 and self.grab is not None:
                st = self.grab.get("grapple")
                if st is not None and ("Grappled" in st or "Ready" in st):
                    self.success = True

    def test(self, w):
        if self.first:
            self.parts_changed(w)
            self.first = False
        return self.success

    def body_text(self, field):
        if field["name"] == "grapplingVesselID" and field["has"]:
            t = fmt("#autoLOC_8100159") if self.gv != 0 else fmt("#autoLOC_8004217")
            return fmt("#autoLOC_8004190", fmt("#autoLOC_8000001"), t)
        if field["name"] == "dockedUndocked" and field["has"]:
            if field["value"] == "Docked":
                return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8002014"))
            return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8002015"))
        return self.base_body(field)

    def vessel_id_changed(self, old, new):
        for a in ("gv", "aid", "cid", "vid"):
            if getattr(self, a) == old:
                setattr(self, a, new)

    def vessel_docking(self, old, new):
        if self.gv == old:
            self.gv = new


# ---- ScienceExperiment -----------------------------------------------------------------------------------------------------------------
class Science(TM):
    NAME = "TestScienceExperiment"
    TITLE = "#autoLOC_8000132"
    INFO = "#autoLOC_8004030"

    def __init__(self):
        super().__init__()
        self.exp = ""
        self.sit = ""
        self.biome = (HOME, "")
        self.dr = "Deployed"
        self.ok = False

    def load(self, cfg):
        self.load_base(cfg)
        self.exp = ""
        self.sit = ""
        v = cfg.get("experimentID")
        if v is not None:
            self.exp = v
        v = cfg.get("experimentSituation")
        if v is not None:
            self.sit = v
        v = cfg.get("testsSuccessful")
        if v is not None:
            self.ok = parse_bool(v)
        v = cfg.get("DeployorReceived")
        if v is not None:
            self.dr = v
        for n in cfg.nodes:
            if n.name == "BIOMEDATA":
                bn = n.get("bodyName")
                bi = n.get("biomeName")
                if bn is not None and bn != "":
                    self.biome = (body_by_name_or_none(bn), self.biome[1])
                if bi is not None and bi != "":
                    self.biome = (self.biome[0], bi)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        if self.exp:
            out.values.append(("experimentID", self.exp))
        if self.sit:
            out.values.append(("experimentSituation", self.sit))
        out.values += [("testsSuccessful", b(self.ok)), ("DeployorReceived", self.dr)]
        bn = Cfg("BIOMEDATA")
        bn.values.append(("bodyName", body(self.biome[0])["name"]))
        if self.biome[1]:
            bn.values.append(("biomeName", self.biome[1]))
        out.nodes.append(bn)
        return out

    def initialized(self):
        pass

    def conduct(self, sid):
        passed = True
        if self.exp and sid.split("@")[0] != self.exp:
            passed = False
        if self.sit and self.sit not in sid:
            passed = False
        if self.biome[1]:
            if self.biome[0] is not None and body(self.biome[0])["name"] not in sid:
                passed = False
            if self.biome[1] not in sid:
                passed = False
        self.ok = passed

    def received(self, sid):
        if not (self.dr != "Received"):
            self.conduct(sid)

    def stored(self, sid):
        if not (self.dr != "Deployed"):
            self.conduct(sid)

    def body_text(self, field):
        n = field["name"]
        if n == "biomeData":
            bd = body(self.biome[0])
            if self.biome[1]:
                return fmt("#autoLOC_8000255", bd["display"], self.biome[1])
            return fmt("#autoLOC_8000269", bd["display"])
        if n == "DeployorReceived":
            return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8001037") if self.dr == "Deployed" else fmt("#autoLOC_8000250"))
        return self.base_body(field)


# ---- scenario generators -------------------------------------------------------------------------------------------------------------
def head_ops(m):
    return [{"op": "title", "out": m.title}, {"op": "info", "out": m.info()}, {"op": "saved", "out": m.save().dump()}]


def body_ops(m, fields):
    ops = []
    for f in fields:
        f = dict(f)
        f.setdefault("value", "x")
        f.setdefault("has", True)
        ops.append({"op": "body", "field": f, "out": m.body_text(f)})
    return ops


def step_test(m, w, ops):
    try:
        res = m.test(w)
        ops.append({"op": "test", "out": res})
    except KeyError:
        ops.append({"op": "test", "out": "throws"})


def new_env(w, ops, ut):
    w["ut"] = ut
    ops.append({"op": "env", "env": w})


def tv_cfg(r, cfg):
    cfg.values.append(("vesselID", rng_pick(r, ["0", "0", "107", "114"])))
    if r.random() < 0.4:
        cfg.values.append(("title", rng_pick(r, ["Custom title", "x"])))


def bias_sit(r, w, sit_choices, area=None, volume=None):
    for v in w["vessels"]:
        if r.random() < 0.75:
            v["sit"] = rng_pick(r, sit_choices)
        if area is not None and area.radius > 0:
            v["mainBody"] = area.body
            v["orbit"]["ref"] = area.body
            pos = pos_near(r, area, r.random() < 0.55)
            if pos:
                v["lat"], v["lon"] = pos
        if volume is not None:
            v["mainBody"] = volume.body
            v["orbit"]["ref"] = volume.body
            v["lat"] = max(-85.0, min(85.0, volume.lat + r.uniform(-2.0, 2.0)))
            v["lon"] = volume.lon + r.uniform(-2.0, 2.0)
            v["alt"] = f32(rng_pick(r, [0.0, volume.hmin + 1.0, volume.hmax - 1.0, volume.hmax + 5000.0, volume.hs, volume.hs + 1000.0, 2500.0]))
            if not volume.margin_ok(v["lat"], v["lon"], v["alt"]):
                v["alt"] = f32(volume.hmax * 3.0 + 7.0)


def gen_goto(r):
    m = GoTo()
    cfg = Cfg("N")
    tv_cfg(r, cfg)
    for k in r.sample(["radius", "alt", "lon", "lat", "bodyName"], r.randrange(1, 6)):
        cfg.values.append((k, rng_pick(r, ["150000", "500000", "2000000"]) if k == "radius" else (rng_pick(r, ["Kerbin", "Mun"]) if k == "bodyName" else rng_pick(r, ["0", "10.5", "-20", "30.25"]))))
    if r.random() < 0.6:
        cfg.values.append(("surfaceVelocity", rng_pick(r, ["0.1", "1", "5.5", "0"])))
    if r.random() < 0.5:
        cfg.values.append(("ignoreSurfaceVelocity", rng_pick(r, ["True", "False"])))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "areaData", "guiName": "#autoLOC_8000264", "fieldType": "SurfaceArea"}])
    ut = 100.0
    for step in range(8):
        w = gen_world(r)
        bias_sit(r, w, ["LANDED", "SPLASHED", "FLYING"], area=m.area)
        new_env(w, ops, ut)
        for k in range(3):
            step_test(m, w, ops)
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, m.resource_vessel(w)), "number": None}})
        if m.vessel is not None and m.area.radius > 0:
            d = m.area.dist(m.vessel["lat"], m.vessel["lon"])
            ops.append({"op": "scoreMod", "module": "ScoreModule_Accuracy", "approx": True,
                        "out": {"vessel": None, "number": 1.0 - max(0.0, min(1.0, d / m.area.radius))}})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_flythrough(r):
    m = FlyThrough()
    cfg = Cfg("N")
    tv_cfg(r, cfg)
    for k in r.sample(["hConeMax", "alt", "hConeMin", "hSphere", "lat", "lon", "shape", "targetBody", "rad"], r.randrange(2, 10)):
        if k in ("hConeMax", "hConeMin", "hSphere", "rad"):
            cfg.values.append((k, rng_pick(r, ["150000", "5000.5", "100000", "0", "30000"])))
        elif k == "shape":
            cfg.values.append((k, rng_pick(r, ["Cone", "Sphere"])))
        elif k == "targetBody":
            cfg.values.append((k, rng_pick(r, ["Kerbin", "Mun"])))
        else:
            cfg.values.append((k, rng_pick(r, ["0", "10.5", "-20", "30.25"])))
    cfg.values.append(("showNodeLabel", rng_pick(r, ["True", "False"])))
    cfg.values.append(("showMapNode", rng_pick(r, ["True", "False"])))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "volumeData", "guiName": "#autoLOC_8000146", "fieldType": "SurfaceVolume"}])
    for step in range(8):
        w = gen_world(r)
        bias_sit(r, w, SIT_NAMES, volume=m.vol)
        new_env(w, ops, 100.0)
        for k in range(3):
            step_test(m, w, ops)
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, m.resource_vessel(w)), "number": None}})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_plantflag(r):
    m = PlantFlag()
    cfg = gen_surf_cfg(r)
    cfg.values.append(("eventFound", rng_pick(r, ["True", "False"])))
    if r.random() < 0.4:
        cfg.values.append(("title", "Plant it"))
    m.load(cfg)
    ops = head_ops(m)
    ops.append({"op": "init"})
    m.initialized()
    for step in range(6):
        w = gen_world(r)
        bias_sit(r, w, ["LANDED"], area=m.s.area if m.s.choice == "areaData" else None)
        new_env(w, ops, 100.0)
        ops.append({"op": "test", "out": m.found})
        if r.random() < 0.8 and w["vessels"]:
            v = rng_pick(r, w["vessels"])
            ops.append({"op": "event", "name": "flagPlanted", "vessel": v["pid"]})
            m.planted(v)
        ops.append({"op": "test", "out": m.found})
        if r.random() < 0.3:
            ops.append({"op": "clear"})
            ops.append({"op": "init"})
            m.initialized()
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, active(w)), "number": None}})
        if m.s.choice == "areaData" and m.v is not None:
            d = m.s.area.dist(m.v["lat"], m.v["lon"])
            acc = 1.0 - max(0.0, min(1.0, d / m.s.area.radius)) if m.s.area.radius > 0 else None
            if acc is not None:
                ops.append({"op": "scoreMod", "module": "ScoreModule_Accuracy", "approx": True, "out": {"vessel": None, "number": acc}})
        elif m.s.choice != "areaData":
            ops.append({"op": "scoreMod", "module": "ScoreModule_Accuracy", "out": {"vessel": None, "number": 1.0}})
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_crashed(r):
    m = Crashed()
    cfg = gen_surf_cfg(r)
    tv_cfg(r, cfg)
    m.load(cfg)
    ops = head_ops(m)
    ops.append({"op": "init"})
    m.initialized()
    for step in range(6):
        w = gen_world(r)
        bias_sit(r, w, SIT_NAMES, area=m.s.area if m.s.choice == "areaData" else None)
        if m.s.choice == "areaData" and not all(margin_ok(m.s, v) for v in w["vessels"]):
            continue
        new_env(w, ops, 100.0)
        ops.append({"op": "test", "out": m.found})
        if w["vessels"]:
            v = rng_pick(r, w["vessels"])
            ops.append({"op": "event", "name": "vesselWillDestroy", "vessel": v["pid"]})
            m.destroyed(w, v)
        ops.append({"op": "test", "out": m.found})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, m.resource_vessel(w)), "number": None}})
        if r.random() < 0.3:
            ops.append({"op": "init"})
            m.initialized()
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_velocity(r):
    m = Velocity()
    cfg = Cfg("N")
    tv_cfg(r, cfg)
    cfg.values.append(("velocity", rng_pick(r, ["0", "100", "2200.5", "30.25", "5000"])))
    cfg.values.append(("comparisonOperator", rng_pick(r, ["LessThan", "GreaterThan"])))
    cfg.values.append(("speedType", rng_pick(r, ["SurfaceVelocity", "OrbitalVelocity"])))
    cfg.values.append(("referenceBody", rng_pick(r, ["Default", "Asteroid", "Comet", "Kerbal", "Vessel"])))
    cfg.values.append(("velocityVesselID", rng_pick(r, ["107", "114", "999"])))
    cfg.values.append(("velocityAsteroidID", rng_pick(r, ["107", "114", "999"])))
    cfg.values.append(("velocityCometID", rng_pick(r, ["107", "114"])))
    kn = Cfg("VELOCITYWRTKERBAL")
    gen_mk_cfg(r, kn)
    cfg.nodes.append(kn)
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "velocity", "guiName": "#autoLOC_8004169"}])
    for step in range(8):
        w = gen_world(r, nv=r.randrange(1, 4))
        new_env(w, ops, 100.0)
        for k in range(3):
            step_test(m, w, ops)
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_orbit(r):
    m = OrbitT()
    cfg = Cfg("N")
    tv_cfg(r, cfg)
    if r.random() < 0.7:
        cfg.values.append(("orbitAccuracy", rng_pick(r, ["90", "50", "99.5", "0", "100"])))
    else:
        cfg.values.append(("orbitTolerance", rng_pick(r, ["0.1", "0.25", "0.5"])))
    cfg.values.append(("stabilizationTime", rng_pick(r, ["0", "1", "5"])))
    cfg.values.append(("underThrust", rng_pick(r, ["True", "False"])))
    on = Cfg("ORBIT")
    for k in ("SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH"):
        on.values.append((k, rng_pick(r, ref_data_pool(k))))
    on.values.append(("REF", str(rng_pick(r, [0, 1, 2]))))
    cfg.nodes.append(on)
    m.load(cfg)
    ops = head_ops(m)
    ops += body_ops(m, [{"name": "missionOrbit", "guiName": "#autoLOC_8000163", "fieldType": "MissionOrbit"}, {"name": "orbitAccuracy", "guiName": "#autoLOC_8000159"}])
    ops.append({"op": "init"})
    m.initialized()
    ut = 100.0
    for step in range(8):
        w = gen_world(r)
        for v in w["vessels"]:
            if r.random() < 0.6:
                v["orbit"]["ref"] = m.orb.orb_ref
                v["mainBody"] = m.orb.orb_ref
                v["orbit"]["sma"] = m.orb.orb_sma * rng_pick(r, [1.0, 1.01, 1.2, 0.5])
                v["gee"] = rng_pick(r, [0.0, 0.05, 0.5])
        new_env(w, ops, ut)
        for k in range(3):
            w["ut"] = ut = ut + rng_pick(r, [0.0, 3.0, 10.0])
            ops.append({"op": "setUT", "ut": ut})
            step_test(m, w, ops)
        if m.vesselID == 0 or vessel_by_pid(w, m.vesselID) is not None:
            rv = active(w) if m.vesselID == 0 else vessel_by_pid(w, m.vesselID)
            ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, rv), "number": None}})
        else:
            ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "throws": True})
        if m.vessel is not None and m.rel is not None:
            ops.append({"op": "scoreMod", "module": "ScoreModule_Accuracy", "out": {"vessel": None, "number": synth_at_orbit_acc(m.rel, m.deviation(), m.vessel)}})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "orbitHook": True}


def ref_data_pool(k):
    from ref_data import ORB_POOL
    return ORB_POOL[k]


def gen_orbitparams(r):
    m = OrbitParams()
    cfg = Cfg("N")
    tv_cfg(r, cfg)
    for k, vals in (("checkApoapsis", ["True", "False"]), ("checkPeriapsis", ["True", "False"]), ("checkInclination", ["True", "False"]), ("checkLAN", ["True", "False"]),
                    ("apMinValue", ["0", "70000", "100000"]), ("apMaxValue", ["100000", "150000", "5000000"]), ("peMinValue", ["0", "-50000", "70000"]), ("peMaxValue", ["100000", "250000"]),
                    ("inclination", ["0", "28.5", "90", "-45"]), ("inclinationAccuracy", ["90", "50", "99"]), ("LAN", ["0", "100", "270.25"]), ("LANAccuracy", ["90", "50", "95"]),
                    ("stabilizationTime", ["0", "2", "5"]), ("underThrust", ["True", "False"])):
        if r.random() < 0.75:
            cfg.values.append((k, rng_pick(r, vals)))
    if r.random() < 0.6:
        cfg.values.append(("bodyName", rng_pick(r, ["Kerbin", "Mun", "AnyValid"])))
    m.load(cfg)
    ops = head_ops(m)
    ops += body_ops(m, [{"name": n, "guiName": "#autoLOC_8100059"} for n in ("checkApoapsis", "checkPeriapsis", "checkInclination", "checkLAN", "body", "apMinValue", "stabilizationTime")])
    ut = 100.0
    for step in range(10):
        w = gen_world(r)
        for v in w["vessels"]:
            if r.random() < 0.6 and m.body is not None:
                v["orbit"]["ref"] = m.body
                v["mainBody"] = m.body
            v["gee"] = rng_pick(r, [0.0, 0.05, 0.5])
            v["protoSit"] = rng_pick(r, ["ORBITING", "ESCAPING", "SUB_ORBITAL", "LANDED"])
            if r.random() < 0.7:
                v["sit"] = rng_pick(r, ["ORBITING", "ESCAPING", "SUB_ORBITAL", "FLYING"])
        new_env(w, ops, ut)
        for k in range(3):
            w["ut"] = ut = ut + rng_pick(r, [0.0, 3.0, 10.0])
            ops.append({"op": "setUT", "ut": ut})
            step_test(m, w, ops)
        if m.vessel is not None:
            ops.append({"op": "scoreMod", "module": "ScoreModule_Accuracy", "approx": True, "out": {"vessel": None, "number": m.accuracy(m.vessel)}})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, m.resource_vessel(w)), "number": None}})
    ops.append({"op": "validate", "out": m.validation()})
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def gen_world_ids(r, w):
    """give the world some part ids / flags used by the part-aware tests"""
    for v in w["vessels"]:
        for p in v["parts"]:
            if r.random() < 0.3:
                p["ast"] = True
            if r.random() < 0.4:
                p["grapple"] = rng_pick(r, ["Ready", "Grappled (Docked)", "Disabled", "Disengaged"])
    return w


def gen_crew_assignment(r):
    m = CrewAssign()
    cfg = Cfg("N")
    cfg.values.append(("partPersistentId", rng_pick(r, ["0", "0", "1000", "1070", "1140", "5"])))
    cfg.values.append(("vesselPersistentId", rng_pick(r, ["100", "107", "114", "999"])))
    cfg.values.append(("partName", rng_pick(r, ["#autoLOC_8001004", "Any", "mk1pod"])))
    gen_mk_cfg(r, cfg)
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "missionKerbal", "guiName": "#autoLOC_8007219"}, {"name": "vesselID", "guiName": "#autoLOC_8000001"},
                                     {"name": "vesselPartIDs", "guiName": "#autoLOC_8000012"}])
    for step in range(8):
        w = gen_world(r, nv=r.randrange(1, 4))
        new_env(w, ops, 100.0)
        for k in range(2):
            step_test(m, w, ops)
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource",
                    "out": {"vessel": vtext(w, vessel_by_pid(w, m.pair.vid) or active(w)), "number": None}})
        if r.random() < 0.3 and w["vessels"]:
            v = rng_pick(r, w["vessels"])
            old, new = m.pair.vid, v["pid"] + 1
            ops.append({"op": "call", "fn": "OnVesselPersistentIdChanged", "a": [old, new]})
            m.vessel_id_changed(old, new)
            ops.append({"op": "saved", "out": m.save().dump()})
        if r.random() < 0.3 and len(w["vessels"]) >= 2:
            a, c = w["vessels"][0], w["vessels"][1]
            ops.append({"op": "call", "fn": "OnVesselsUndocking", "a": [a["pid"], c["pid"]]})
            m.undocking(a, c)
            ops.append({"op": "saved", "out": m.save().dump()})
        if r.random() < 0.3:
            ops.append({"op": "call", "fn": "OnPartPersistentIdChanged", "a": [m.pair.vid, m.pair.pid, m.pair.pid + 3]})
            m.part_id_changed(m.pair.vid, m.pair.pid, m.pair.pid + 3)
            ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "partIds": True}


def gen_partdock(r):
    m = PartDock()
    cfg = Cfg("N")
    for nm in ("PARTONE", "PARTTWO"):
        n = Cfg(nm)
        n.values.append(("partPersistentId", rng_pick(r, ["0", "1000", "1070", "1140", "1210"])))
        n.values.append(("vesselPersistentId", rng_pick(r, ["100", "107", "114"])))
        n.values.append(("partName", rng_pick(r, ["#autoLOC_8001004", "Any", "mk1pod"])))
        cfg.nodes.append(n)
    cfg.values.append(("dockedUndocked", rng_pick(r, ["Docked", "Undocked"])))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "partOnevesselPartIDs", "guiName": "#autoLOC_8002008"}, {"name": "partTwovesselPartIDs", "guiName": "#autoLOC_8002010"},
                                     {"name": "dockedUndocked", "guiName": "#autoLOC_8002012", "value": m.dock}])
    ops.append({"op": "init"})
    m.initialized()
    for step in range(5):
        w = gen_world(r, nv=r.randrange(2, 4))
        new_env(w, ops, 100.0)
        for k in range(2):
            step_test(m, w, ops)
        if r.random() < 0.5:
            ops.append({"op": "event", "name": "partCouple" if m.dock == "Docked" else "partUndock"})  # only the matching events are subscribed
            m.parts_changed(w)
            ops.append({"op": "test", "out": m.success})
        if r.random() < 0.3:
            ops.append({"op": "call", "fn": "OnVesselDocking", "a": [m.p1.vid, 1]})
            m.vessel_docking(w, m.p1.vid, 1)
            ops.append({"op": "saved", "out": m.save().dump()})
        if r.random() < 0.3:
            ops.append({"op": "call", "fn": "OnVesselPersistentIdChanged", "a": [m.p2.vid, m.p2.vid + 1]})
            m.vessel_id_changed(m.p2.vid, m.p2.vid + 1)
            ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "partIds": True}


def gen_grapple(r):
    m = Grapple()
    cfg = Cfg("N")
    if r.random() < 0.5:
        n = Cfg("PARTONE")
        n.values.append(("vesselPersistentId", rng_pick(r, ["100", "107"])))
        cfg.nodes.append(n)
    for k in ("grapplingVesselID", "grappleAsteroidID", "grappleCometID", "grappleVesselID"):
        if r.random() < 0.8:
            cfg.values.append((k, rng_pick(r, ["0", "100", "107", "114", "999"])))
    cfg.values.append(("grappledSpaceObject", rng_pick(r, ["Asteroid", "Comet", "Vessel"])))
    cfg.values.append(("dockedUndocked", rng_pick(r, ["Docked", "Undocked"])))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "grapplingVesselID", "guiName": "#autoLOC_8005456"}, {"name": "dockedUndocked", "guiName": "#autoLOC_8002012", "value": m.dock}])
    ops.append({"op": "init"})
    m.initialized()
    for step in range(5):
        w = gen_world_ids(r, gen_world(r, nv=r.randrange(2, 4)))
        new_env(w, ops, 100.0)
        for k in range(2):
            step_test(m, w, ops)
        if r.random() < 0.6:
            ops.append({"op": "event", "name": "partUndock" if m.dock != "Docked" else "partCouple"})
            m.parts_changed(w)
            ops.append({"op": "test", "out": m.success})
        if r.random() < 0.3:
            ops.append({"op": "call", "fn": "OnVesselPersistentIdChanged", "a": [m.gv, m.gv + 1]})
            m.vessel_id_changed(m.gv, m.gv + 1)
            ops.append({"op": "saved", "out": m.save().dump()})
        if r.random() < 0.3:
            ops.append({"op": "call", "fn": "OnVesselDocking", "a": [m.gv, 4242]})
            m.vessel_docking(m.gv, 4242)
            ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "partIds": True}


def gen_science(r):
    m = Science()
    cfg = Cfg("N")
    if r.random() < 0.7:
        cfg.values.append(("experimentID", rng_pick(r, ["crewReport", "temperatureScan", "mysteryGoo"])))
    if r.random() < 0.6:
        cfg.values.append(("experimentSituation", rng_pick(r, ["SrfLanded", "InSpaceLow", "FlyingHigh"])))
    cfg.values.append(("DeployorReceived", rng_pick(r, ["Deployed", "Received"])))
    if r.random() < 0.5:
        cfg.values.append(("testsSuccessful", rng_pick(r, ["True", "False"])))
    bn = Cfg("BIOMEDATA")
    bn.values.append(("bodyName", rng_pick(r, ["Kerbin", "Mun", "Minmus"])))
    if r.random() < 0.6:
        bn.values.append(("biomeName", rng_pick(r, ["Shores", "Highlands", "Grasslands"])))
    cfg.nodes.append(bn)
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, [{"name": "biomeData", "guiName": "#autoLOC_8000136"}, {"name": "DeployorReceived", "guiName": "#autoLOC_8000137"}])
    ops.append({"op": "init"})
    m.initialized()
    for step in range(8):
        w = gen_world(r)
        new_env(w, ops, 100.0)
        sid = "%s@%s%s%s" % (rng_pick(r, ["crewReport", "temperatureScan", "mysteryGoo"]), rng_pick(r, ["Kerbin", "Mun", "Minmus"]),
                             rng_pick(r, ["SrfLanded", "InSpaceLow", "FlyingHigh"]), rng_pick(r, ["", "Shores", "Highlands", "Grasslands"]))
        if r.random() < 0.5:
            ops.append({"op": "event", "name": "scienceReceived", "id": sid})
            m.received(sid)
        else:
            ops.append({"op": "event", "name": "experimentStored", "id": sid})
            m.stored(sid)
        ops.append({"op": "test", "out": m.ok})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, active(w)), "number": None}})
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops}


def generate(r):
    cases = []
    for _ in range(30):
        cases.append(gen_goto(r))
        cases.append(gen_flythrough(r))
        cases.append(gen_plantflag(r))
        cases.append(gen_crashed(r))
        cases.append(gen_velocity(r))
        cases.append(gen_orbit(r))
        cases.append(gen_orbitparams(r))
        cases.append(gen_crew_assignment(r))
        cases.append(gen_partdock(r))
        cases.append(gen_grapple(r))
        cases.append(gen_science(r))
    return cases
