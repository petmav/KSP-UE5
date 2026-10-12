"""WP-54 slice 2 reference, part 3: the Tests.* classes WP-55 deferred (Expansions/Missions/Tests/*.cs). No kspcore code.
Each model is written from its C# file; the scenarios are labelled synthetic (a seeded vessel world, event sequences)."""
import math

from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, f32, fnum, fmt, tag, b, g9, soi, strip_gender, custom_0hh, custom_0, custom_0h0, nfmt, KSPRandom, guid_of
from ref_data import (D, F, FS, rng_pick, Area, Volume, Orb, VGL, great_circle, body, body_by_name_or_none, HOME, BODIES, POOL_NUM, CLASSES, mathf_approx, parse_f,
                      uint_try, gen_surface)  # noqa: F401
import ref_data

SIT_NAMES = ["LANDED", "SPLASHED", "FLYING", "PRELAUNCH", "ORBITING", "SUB_ORBITAL", "ESCAPING", "DOCKED"]
VESSEL_TYPES = ["Ship", "Probe", "EVA", "Flag", "Debris", "SpaceObject", "Station", "Lander"]
KERBALS = ["Jebediah Kerman", "Bill Kerman", "Bob Kerman", "Valentina Kerman", "Gus Kerman"]
TRAITS = ["Pilot", "Engineer", "Scientist"]
UT_POOL = [0.0, 10.0, 100.0, 1000.0, 2500.5]


def biome_at(body_idx, lat, lon):
    """the synthetic ScienceUtil.GetExperimentBiome stand-in installed on both sides (the real one needs the biome map)"""
    if lat < -30.0:
        return "Shores"
    if lat < 30.0:
        return "Grasslands"
    return "Highlands"


def convert_tag(t):
    """MiniBiome.ConvertTagtoLandedAt"""
    if not t:
        return ""
    if "Pad" not in t and "LaunchPad" not in t:
        if "Runway" in t:
            return "Runway"
        text = t
        if "KSC" in t and t != "KSC":
            text = text.replace("KSC", "")
        if "Grounds" in t and t != "Grounds":
            text = text.replace("Grounds", "")
        return text.replace("_", " ").strip()
    return "LaunchPad"


# ---- the world -------------------------------------------------------------------------------------------------------------------
def gen_vessel(r, i, area=None):
    ref = r.randrange(0, 3)
    sit = rng_pick(r, SIT_NAMES)
    orb = {"inc": rng_pick(r, [0.0, 28.5, 90.0, -45.0, 120.25]), "ecc": rng_pick(r, [0.0, 0.25, 0.5]), "sma": rng_pick(r, [900000.0, 1500000.0, 700000.0]),
           "lan": rng_pick(r, [0.0, 10.5, 270.25, 100.0]), "argpe": rng_pick(r, [0.0, 45.5]), "mna": rng_pick(r, [0.0, 1.5]), "epoch": 0.0, "ref": ref}
    parts = []
    names = r.sample(KERBALS, r.randrange(0, 4))
    for k in range(r.randrange(1, 4)):
        parts.append({"pid": (100 + 7 * i) * 10 + k, "cap": rng_pick(r, [0, 1, 2]), "crew": []})
    for n in names:
        rng_pick(r, parts)["crew"].append(n)
    v = {"pid": 100 + 7 * i, "id": guid_of(500 + i), "type": rng_pick(r, VESSEL_TYPES), "sit": sit, "lat": rng_pick(r, [-45.5, 0.0, 12.25, 74.5, -74.5625, 33.25]),
         "lon": rng_pick(r, [-45.5, 0.0, 12.25, 74.5, 90.0]), "alt": rng_pick(r, [0.0, 100.0, 5000.5, 80000.0]), "srf": rng_pick(r, [0.0, 0.05, 0.1, 0.3, 25.0]),
         "obt": rng_pick(r, [0.0, 2200.5, 3000.25]), "gee": rng_pick(r, [0.0, 0.05, 0.5, 1.0]), "loaded": r.random() < 0.75, "landedAt": rng_pick(r, ["", "LaunchPad", "KSC_Runway", "Island_Airfield", "Shores"]),
         "crash": rng_pick(r, [None, None, "LaunchPad", "Pad7"]), "obtVel": [f32(rng_pick(r, [0.0, 100.5, -2200.25, 3000.0])) for _ in range(3)], "orbit": orb, "parts": parts,
         "disc": rng_pick(r, ["Owned", "Unowned"]), "protoSit": rng_pick(r, ["ORBITING", "ESCAPING", "SUB_ORBITAL", "LANDED"]), "mainBody": ref}
    return v


def gen_world(r, nv=None):
    nv = nv if nv is not None else r.randrange(1, 4)
    vs = [gen_vessel(r, i) for i in range(nv)]
    return {"ut": rng_pick(r, UT_POOL), "vessels": vs, "active": r.randrange(-1, nv), "mainBody": rng_pick(r, [None, 0, 1, 2]),
            "shipObtVel": [f32(rng_pick(r, [0.0, 100.5, 2000.25])) for _ in range(3)]}


def vessel_by_pid(w, pid):
    for v in w["vessels"]:
        if v["pid"] == pid:
            return v
    return None


def active(w):
    return w["vessels"][w["active"]] if 0 <= w["active"] < len(w["vessels"]) else None


def pos_near(r, area, inside):
    """a (lat, lon) inside / outside the area with a margin to the boundary"""
    for _ in range(50):
        if inside:
            lat, lon = area.lat + r.uniform(-1.0, 1.0), area.lon + r.uniform(-1.0, 1.0)
        else:
            lat, lon = r.uniform(-80.0, 80.0), r.uniform(-170.0, 170.0)
        lat = max(-85.0, min(85.0, lat))
        d = area.dist(lat, lon)
        if area.radius > 0 and abs(d - area.radius) > 1e-3 * area.radius and (d <= area.radius) == inside:
            return lat, lon
    return None


# ---- the TestModule / TestVessel bases -------------------------------------------------------------------------------------------
class TM:
    NAME = ""
    TITLE = None
    TITLE_RAW = False
    INFO = None

    def __init__(self):
        self.title = (self.TITLE if self.TITLE_RAW else fmt(self.TITLE)) if self.TITLE else ""
        self.sap = []
        self.vessel = None

    def load_base(self, cfg):
        t = cfg.get("title")
        if t is not None:
            self.title = t
        self.sap = cfg.get_all("parametersDisplayedInSAP")

    def save_base(self, out):
        out.values += [("name", self.NAME), ("title", self.title)] + [("parametersDisplayedInSAP", s) for s in self.sap]

    def info(self):
        return fmt(self.INFO)

    def base_body(self, field):
        text = tag("#autoLOC_6003000")
        if field["has"]:
            text = field["value"]
        return fmt("#autoLOC_8004190", field["guiName"], text)


class TV(TM):
    def __init__(self):
        super().__init__()
        self.vesselID = 0
        self.useActive = True

    def load_vessel(self, cfg):
        v = cfg.get("vesselID")
        if v is not None:
            self.vesselID = int(v)

    def save_vessel(self, out):
        out.values.append(("vesselID", str(self.vesselID)))

    def tv_test(self, w):
        if self.vesselID == 0:
            self.vessel = active(w)
        else:
            v = vessel_by_pid(w, self.vesselID)
            if v is not None:
                self.vessel = v
            elif self.vessel is not None:
                self.vessel = None
        return True

    def resource_vessel(self, w):
        if self.vesselID == 0:
            return active(w)
        return vessel_by_pid(w, self.vesselID)


def vtext(w, v):
    return None if v is None else v["pid"]


# ---- surface choices model (shared by the location tests) -----------------------------------------------------------------------------
class Surf:
    """ParamChoices_CelestialBodySurface as the tests build it in Awake"""

    def __init__(self, any_body, choice="bodyData"):
        self.choice = choice
        self.body = None if any_body else HOME
        self.biome = (HOME, "")
        self.area = Area()
        self.area.body = HOME
        self.area.radius = 150000.0
        self.site = ""

    def load(self, cfg):
        c = cfg.get("locationChoice")
        self.choice = c if c in ("anyData", "bodyData", "biomeData", "areaData", "launchSiteName") else "bodyData"
        for n in cfg.nodes:
            if n.name == "BODYDATA":
                bn = n.get("bodyName")
                if bn is not None and bn != "":
                    self.body = None if bn == "AnyValid" else body_by_name_or_none(bn)
            elif n.name == "BIOMEDATA":
                bn = n.get("bodyName")
                bi = n.get("biomeName")
                if bn is not None and bn != "":
                    self.biome = (body_by_name_or_none(bn), self.biome[1])
                if bi is not None and bi != "":
                    self.biome = (self.biome[0], bi)
            elif n.name == "GROUNDAREA":
                self.area.load(n)
        v = cfg.get("LAUNCHSITEDATA")
        if v is not None:
            self.site = v

    def save(self, out):
        out.values.append(("locationChoice", self.choice))
        nb = Cfg("BODYDATA")
        nb.values.append(("bodyName", "AnyValid" if self.body is None else body(self.body)["name"]))
        out.nodes.append(nb)
        ni = Cfg("BIOMEDATA")
        ni.values.append(("bodyName", body(self.biome[0])["name"]))
        if self.biome[1]:
            ni.values.append(("biomeName", self.biome[1]))
        out.nodes.append(ni)
        na = Cfg("GROUNDAREA")
        self.area.save(na)
        out.nodes.append(na)
        out.values.append(("LAUNCHSITEDATA", self.site))

    def valid_body(self, mb):
        return self.body is None or self.body == mb

    def has_waypoint(self):
        return self.choice == "areaData"


def gen_surf_cfg(r):
    cfg = Cfg("N")
    if r.random() < 0.9:
        cfg.values.append(("locationChoice", rng_pick(r, ["anyData", "bodyData", "biomeData", "areaData", "launchSiteName"])))
    if r.random() < 0.6:
        n = Cfg("BODYDATA")
        n.values.append(("bodyName", rng_pick(r, ["Kerbin", "Mun", "AnyValid"])))
        cfg.nodes.append(n)
    if r.random() < 0.6:
        n = Cfg("BIOMEDATA")
        n.values.append(("bodyName", rng_pick(r, ["Kerbin", "Mun"])))
        if r.random() < 0.7:
            n.values.append(("biomeName", rng_pick(r, ["Shores", "grasslands", "Highlands"])))
        cfg.nodes.append(n)
    if r.random() < 0.7:
        n = Cfg("GROUNDAREA")
        n.values.append(("lat", rng_pick(r, ["0", "10.5", "-20"])))
        n.values.append(("lon", rng_pick(r, ["0", "30.25", "-80"])))
        n.values.append(("radius", rng_pick(r, ["150000", "500000", "2000000"])))
        n.values.append(("bodyName", "Kerbin"))
        cfg.nodes.append(n)
    if r.random() < 0.6:
        cfg.values.append(("LAUNCHSITEDATA", rng_pick(r, ["LaunchPad", "Runway", "Pad7", ""])))
    return cfg


def location_ok(s, v):
    """does the location condition hold for vessel dict v (the Test() switch of TestVesselSituation / TestVesselStateCrashed)"""
    mb = v["mainBody"]
    if s.choice == "bodyData":
        return s.valid_body(mb), None
    if s.choice == "biomeData":
        cur = biome_at(mb, v["lat"], v["lon"])
        return (s.biome[1] == "" or cur.lower() == s.biome[1].lower()), None
    if s.choice == "areaData":
        if s.area.body == mb:
            return s.area.dist(v["lat"], v["lon"]) <= s.area.radius, "area"
        return None, "area"
    if s.choice == "launchSiteName":
        if v["crash"] is not None:
            return s.site == v["crash"], None
        return s.site == convert_tag(v["landedAt"]), None
    return False, None


def margin_ok(s, v):
    if s.choice != "areaData" or s.area.body != v["mainBody"]:
        return True
    d = s.area.dist(v["lat"], v["lon"])
    return s.area.radius > 0 and abs(d - s.area.radius) > 1e-3 * s.area.radius


# ---- TestVesselSituation family ----------------------------------------------------------------------------------------------------------
class SitTest(TV):
    KINDS = {"TestVesselSituationFlying": ("FLYING", "#autoLOC_8000180", "#autoLOC_8004015", True), "TestVesselSituationLanded": ("LANDED", "#autoLOC_8000183", "#autoLOC_8004020", False),
             "TestVesselSituationSplashed": ("SPLASHED", "#autoLOC_8003052", "#autoLOC_8004031", True)}

    def __init__(self, name):
        self.NAME = name
        sit, ttl, info, ign = self.KINDS[name]
        self.TITLE, self.INFO = ttl, info
        super().__init__()
        self.sit = sit
        self.stab = 1.0
        self.speed = 0.10000000149011612
        self.ignore = ign
        self.s = Surf(True)
        self.start = -1.0
        self.loc_success = False

    def load(self, cfg):
        self.load_base(cfg)
        self.load_vessel(cfg)
        v = cfg.get("stabilizationTime")
        if v is not None:
            self.stab = float(v)
        self.s.load(cfg)
        v = cfg.get("surfaceVelocity")
        if v is not None:
            self.speed = float(v)
        v = cfg.get("ignoreSurfaceVelocity")
        if v is not None:
            self.ignore = v.strip().lower() == "true"

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.save_vessel(out)
        out.values.append(("stabilizationTime", D(self.stab)))
        self.s.save(out)
        out.values.append(("surfaceVelocity", D(self.speed)))
        out.values.append(("ignoreSurfaceVelocity", b(self.ignore)))
        return out

    def test(self, w):
        self.tv_test(w)
        v = self.vessel
        ok = v is not None and v["sit"] == self.sit
        if not ok:
            self.start = -1.0
            return False
        res, _ = location_ok(self.s, v)
        if res is not None:
            self.loc_success = res
        elif self.s.choice != "areaData":
            self.loc_success = False
        if not self.loc_success:
            self.start = -1.0
            return False
        if not (self.ignore or v["srf"] <= self.speed + 1.401298464324817e-45):
            self.start = -1.0
            return False
        if self.start < 0.0:
            self.start = w["ut"]
        return self.start + self.stab < w["ut"]

    def accuracy(self, w):
        if self.s.choice == "areaData":
            return self.s.area.dist(self.vessel["lat"], self.vessel["lon"]), self.s.area.radius
        return None

    def body_text(self, field):
        if field["name"] == "surfaceVelocity":
            if self.ignore:
                return fmt("#autoLOC_8004190", field["guiName"], fmt("#autoLOC_8001004"))
            return field["guiName"] + ": " + custom_0h0(self.speed) + fmt("#autoLOC_180095")
        if field["name"] == "stabilizationTime":
            return field["guiName"] + ": " + custom_0(self.stab) + fmt("#autoLOC_6002317")
        return self.base_body(field)


def surf_body_text(s):
    c = s.choice
    if c == "anyData":
        return fmt("#autoLOC_8000272")
    if c == "bodyData":
        return fmt("#autoLOC_8000267", fmt("#autoLOC_8000273") if s.body is None else strip_gender(body(s.body)["display"]))
    if c == "biomeData":
        bd = body(s.biome[0])
        return fmt("#autoLOC_8000268", bd["display"], fmt("#autoLOC_8002051") if not s.biome[1] else s.biome[1])
    if c == "areaData":
        return s.area.text()
    return fmt("#autoLOC_8004190", fmt("#autoLOC_8000067"), s.site)


def gen_world_for(r, models_area=None):
    w = gen_world(r)
    return w


def gen_sit(r):
    name = rng_pick(r, list(SitTest.KINDS.keys()))
    m = SitTest(name)
    cfg = Cfg("N")
    cfg.values.append(("vesselID", rng_pick(r, ["0", "0", "107"])))
    if r.random() < 0.8:
        cfg.values.append(("stabilizationTime", rng_pick(r, ["0", "1", "5.5", "30"])))
    for k in cfg_surf(r):
        cfg.values.append(k)
    scfg = gen_surf_cfg(r)
    cfg.values += scfg.values
    cfg.nodes += scfg.nodes
    if r.random() < 0.6:
        cfg.values.append(("surfaceVelocity", rng_pick(r, ["0.1", "1", "5.5", "0"])))
    if r.random() < 0.6:
        cfg.values.append(("ignoreSurfaceVelocity", rng_pick(r, ["True", "False"])))
    m.load(cfg)
    ops = [{"op": "title", "out": m.title}, {"op": "info", "out": m.info()}, {"op": "saved", "out": m.save().dump()}]
    for field in ({"name": "surfaceVelocity", "guiName": "#autoLOC_8003020", "value": "x", "has": True}, {"name": "stabilizationTime", "guiName": "#autoLOC_8003019", "value": "x", "has": True},
                  {"name": "vesselID", "guiName": "#autoLOC_8000001", "value": "x", "has": True}):
        if field["name"] == "vesselID":
            continue
        ops.append({"op": "body", "field": field, "out": m.body_text(field)})
    w = None
    ut = rng_pick(r, UT_POOL)
    for step in range(8):
        w = gen_world(r)
        # bias the first vessel towards the module's situation / location so the success path runs
        for v in w["vessels"]:
            if r.random() < 0.7:
                v["sit"] = m.sit
        if m.s.choice == "areaData":
            for v in w["vessels"]:
                v["mainBody"] = m.s.area.body if r.random() < 0.8 else v["mainBody"]
                v["orbit"]["ref"] = v["mainBody"]
                if v["mainBody"] == m.s.area.body and m.s.area.radius > 0:
                    pos = pos_near(r, m.s.area, r.random() < 0.6)
                    if pos:
                        v["lat"], v["lon"] = pos
        elif m.s.choice == "bodyData":
            for v in w["vessels"]:
                if m.s.body is not None and r.random() < 0.6:
                    v["mainBody"] = m.s.body
                    v["orbit"]["ref"] = v["mainBody"]
        w["ut"] = ut
        ops.append({"op": "env", "env": w})
        ok = all(margin_ok(m.s, v) for v in w["vessels"])
        if not ok:
            ops.pop()
            continue
        for k in range(3):
            w["ut"] = ut = ut + rng_pick(r, [0.0, 0.5, 2.0, 10.0])
            ops.append({"op": "setUT", "ut": ut})
            exp = m.test(w)
            ops.append({"op": "test", "out": exp})
            if exp and m.vessel is not None:
                pass
        for modname in ("ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"):
            if modname == "ScoreModule_Resource":
                v = m.resource_vessel(w)
                ops.append({"op": "scoreMod", "module": modname, "out": {"vessel": vtext(w, v), "number": None}})
            elif modname == "ScoreModule_Accuracy" and m.vessel is not None and m.vessel["sit"] == m.sit:
                acc = m.accuracy(w)
                num = 1.0 if acc is None else 1.0 - max(0.0, min(1.0, acc[0] / acc[1]))
                ops.append({"op": "scoreMod", "module": modname, "out": {"vessel": None, "number": num}, "approx": acc is not None})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "biome": True}


def cfg_surf(r):
    return []


def generate(r):
    cases = []
    for _ in range(60):
        cases.append(gen_sit(r))
    return cases
