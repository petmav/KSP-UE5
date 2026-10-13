"""WP-54c reference, part 1: TestDistance (Expansions/Missions/Tests/TestDistance.cs). Each rule is written from the C#; scenarios are labelled synthetic
(seeded vessel world with world positions, synthetic body positions / frames, a small mission model). No kspcore code.

Comparison policy (the C++ side): booleans are compared exactly; the scenarios keep a relative margin of 1e-6 between the measured distance and the
threshold (the double libm acos / cos / sin of the great circle and the world surface position differ in the last digits between runtimes)."""
import math

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import (Cfg, f32, fmt, tag, b, guid_of, BODIES, HOME, body, D, rng_pick, VGL, Area, body_by_name_or_none, EMPTY_GUID, MissionModel, mv_index_mapped,
                          world_surface_position, v3_to_f32, vnorm, vsub, vdist, gc_positions, MappedVessel, TICK)
from ref_common import KSPRandom
from ref_tests2 import TM, gen_world, active, vessel_by_pid, vtext, KERBALS
from ref_tests3 import MK, gen_mk_cfg, head_ops, new_env, crew_of, parse_bool, ROSTER_TOURISTS
from ref_actions2 import new_vgl

FROM_CHOICES = ["Kerbal", "Vessel"]
TO_CHOICES = ["Asteroid", "Comet", "CelestialBody", "Flag", "Kerbal", "LaunchSite", "Location", "NodeLabelNode", "Vessel"]
CALC = ["StraightLine", "GreatCircle"]
LG = ["LessThan", "GreaterThan"]
GUIDS = [EMPTY_GUID, guid_of(98), guid_of(61), guid_of(62), guid_of(63)]
SITE_NAMES = ["LaunchPad", "Runway", "Island_Airfield", "KSC2"]


def parse_guid(text):
    import re
    t = text.strip()
    if re.fullmatch(r"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}", t):
        return t.lower()
    return None


def parse_uint(text, cur):
    t = text.strip()
    if t.isdigit() and int(t) < 2 ** 32:
        return int(t)
    return cur


def parse_double(text, cur):
    try:
        return float(text)
    except ValueError:
        return cur


class Dist(TM):
    NAME = "TestDistance"
    TITLE = "#autoLOC_8000176"
    INFO = "#autoLOC_8004141"

    def __init__(self, mission):
        super().__init__()
        self.m = mission
        self.fromT = "Vessel"
        self.fromK = MK("Crew", False, True)
        self.fromV = 0
        self.toT = "Vessel"
        self.toK = MK("Crew", False, False)
        self.toV = 0
        self.loc = new_vgl("Flag")
        self.mcb = HOME           # MissionCelestialBody(FlightGlobals.GetHomeBody()); None = AnyValid
        self.ast = self.comet = self.flag = 0
        self.site = "LaunchPad"
        self.node = EMPTY_GUID
        self.calc = "StraightLine"
        self.cmp = "GreaterThan"
        self.distance = 0.0
        self.fromKV = None        # cached vessels / facility / label (identity semantics: dicts, or tokens)
        self.toKV = None
        self.facility = None
        self.lsite = None
        self.label = None

    # ---- Load / Save (TestDistance.cs:@Save, @Load) -----------------------------------------------------------------------------
    def load(self, cfg):
        self.load_base(cfg)
        v = cfg.get("distanceFromTarget")
        self.fromT = v if v in FROM_CHOICES else "Vessel"
        v = cfg.get("distanceFromVesselID")
        if v is not None:
            self.fromV = parse_uint(v, self.fromV)
        n = next((x for x in cfg.nodes if x.name == "DISTANCEFROMKERBAL"), None)
        if n is not None:
            self.fromK.load(n)
        v = cfg.get("distanceToTarget")
        self.toT = v if v in TO_CHOICES else "Vessel"
        v = cfg.get("distanceToVesselID")
        if v is not None:
            self.toV = parse_uint(v, self.toV)
        v = cfg.get("distanceToLaunchSiteName")
        if v is not None:
            self.site = v
        n2 = next((x for x in cfg.nodes if x.name == "DISTANCETOKERBAL"), None)
        if n2 is not None:
            self.toK.load(n2)
        n3 = next((x for x in cfg.nodes if x.name == "DISTANCETOLOCATION"), None)
        if n2 is not None:          # sic: the C# tests configNode2
            if n3 is None:
                raise KeyError("NRE")
            self.loc.load(n3)
        # MissionCelestialBody.Load
        v = cfg.get("bodyName")
        if v is not None and v != "":
            self.mcb = None if v == "AnyValid" else body_by_name_or_none(v)
        for key, attr in (("distanceToAsteroidID", "ast"), ("distanceToCometID", "comet"), ("distanceToFlagID", "flag")):
            v = cfg.get(key)
            if v is not None:
                setattr(self, attr, parse_uint(v, getattr(self, attr)))
        v = cfg.get("distanceToNode")
        if v is not None:
            g = parse_guid(v)
            self.node = g if g is not None else EMPTY_GUID
        v = cfg.get("calculationType")
        self.calc = v if v in CALC else "StraightLine"
        v = cfg.get("comparisonOperator")
        self.cmp = v if v in LG else "GreaterThan"
        v = cfg.get("distance")
        if v is not None:
            self.distance = parse_double(v, self.distance)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        out.values.append(("distanceFromTarget", self.fromT))
        out.values.append(("distanceFromVesselID", str(self.fromV)))
        n = Cfg("DISTANCEFROMKERBAL")
        self.fromK.save(n)
        out.nodes.append(n)
        out.values.append(("distanceToTarget", self.toT))
        out.values.append(("distanceToVesselID", str(self.toV)))
        self.toK.save(out)
        n2 = Cfg("DISTANCETOKERBAL")
        self.toK.save(n2)
        out.nodes.append(n2)
        n3 = Cfg("DISTANCETOLOCATION")
        self.loc.save(n3)
        out.nodes.append(n3)
        out.values.append(("bodyName", "AnyValid" if self.mcb is None else body(self.mcb)["name"]))
        out.values.append(("distanceToAsteroidID", str(self.ast)))
        out.values.append(("distanceToCometID", str(self.comet)))
        out.values.append(("distanceToFlagID", str(self.flag)))
        out.values.append(("distanceToLaunchSiteName", self.site))
        out.values.append(("distanceToNode", self.node))
        out.values.append(("calculationType", self.calc))
        out.values.append(("comparisonOperator", self.cmp))
        out.values.append(("distance", D(self.distance)))
        return out

    # ---- OnVesselPersistentIdChanged ---------------------------------------------------------------------------------------------
    def vessel_id_changed(self, old, new):
        if self.fromV == old:
            self.fromV = new
        if self.toV == old:
            self.toV = new
        if self.ast == old:
            self.ast = new
        if self.comet == old:
            self.comet = new
        if self.flag == old:
            self.flag = new

    # ---- Test (TestDistance.cs:@Test) --------------------------------------------------------------------------------------------
    def vessel_location(self, w, vid):
        if vid != 0:
            vid = self.m.current_vessel_id(vid)
            if vessel_by_pid(w, vid) is None:
                return None
        v = active(w) if vid == 0 else vessel_by_pid(w, vid)
        if v is None:
            return None
        return v["pos"], v["mainBody"]

    def check_crew(self, v, mk):
        if v is None:
            return False
        for name in reversed(crew_of(v)):
            if mk.is_valid(name, "Crew"):
                return True
        return False

    def kerbal_location(self, w, mk, cached_attr):
        cv = getattr(self, cached_attr)
        if not self.check_crew(cv, mk) or (mk.kerbal is None and cv is not active(w)):
            cv = None
        if cv is None:
            if mk.kerbal is None:
                av = active(w)
                if self.check_crew(av, mk):
                    cv = av
            else:
                for v in w["vessels"]:
                    if cv is not None:
                        break
                    if self.check_crew(v, mk):
                        cv = v
        setattr(self, cached_attr, cv)
        if cv is not None:
            return cv["pos"], cv["mainBody"]
        return None

    def test(self, w):
        if active(w) is None:
            return False
        phys = w["phys"]
        if self.fromT == "Vessel":
            frm = self.vessel_location(w, self.fromV)
        else:
            frm = self.kerbal_location(w, self.fromK, "fromKV")
        loc, cb = (frm if frm is not None else ([0.0, 0.0, 0.0], None))
        flag = frm is not None
        to_loc = [0.0, 0.0, 0.0]
        flag2 = False
        t = self.toT
        if t == "Asteroid":
            if self.ast != 0:
                r = self.vessel_location(w, self.ast)
                if r is not None:
                    to_loc, flag2 = r[0], True
        elif t == "Comet":
            if self.comet != 0:
                r = self.vessel_location(w, self.comet)
                if r is not None:
                    to_loc, flag2 = r[0], True
        elif t == "CelestialBody":
            cb3 = self.mcb if self.mcb is not None else cb
            if cb3 is None:
                raise KeyError("NRE")
            bpos = phys[cb3]["pos"]
            d = vnorm(vsub(loc, bpos))
            to_loc = [bpos[i] + d[i] * BODIES[cb3]["radius"] for i in range(3)]
            flag2 = True
        elif t == "Flag":
            if self.flag != 0:
                r = self.vessel_location(w, self.flag)
                if r is not None:
                    to_loc, flag2 = r[0], True
        elif t == "Kerbal":
            r = self.kerbal_location(w, self.toK, "toKV")
            if r is not None:
                to_loc, flag2 = r[0], True
        elif t == "LaunchSite":
            sites = w.get("sites", {})
            if self.facility is None and self.lsite is None:
                self.facility = sites.get("facility:" + self.site)
            if self.facility is not None:
                to_loc, flag2 = self.facility, True
            if not flag2:
                if self.facility is None and self.lsite is None:
                    self.lsite = sites.get("launchsite:" + self.site)
                if self.lsite is not None:
                    to_loc, flag2 = self.lsite, True
        elif t == "Location":
            to_loc = list(v3_to_f32(world_surface_position(BODIES[self.loc.body], phys[self.loc.body], self.loc.lat, self.loc.lon, 0.0)))
            flag2 = True
        elif t == "NodeLabelNode":
            if self.label is None and self.node != EMPTY_GUID and self.node in self.m.nodes:
                self.label = self.m.node_labels.get(self.node, False)    # False = the node has no ITestNodeLabel module
            if self.label:
                to_loc, flag2 = list(self.label(phys)), True
        elif t == "Vessel":
            r = self.vessel_location(w, self.toV)
            if r is not None:
                to_loc, flag2 = r[0], True
        if not flag or not flag2:
            return False
        return self.points(loc, to_loc, cb, phys)

    def points(self, a, c, cb, phys):
        if self.calc == "GreatCircle":
            if cb is None:
                return False
            num = gc_positions(BODIES[cb]["radius"], phys[cb]["pos"], a, c)
        else:
            num = vdist(a, c)
        if num != num:
            return False
        return num >= self.distance if self.cmp == "GreaterThan" else num <= self.distance

    def measure(self, w):
        """the measured distance of the current state, for the margin check (nan = no measurement)"""
        import copy
        saved = (self.fromKV, self.toKV, self.facility, self.lsite, self.label)
        probe = copy.copy(self)
        probe.fromT, probe.toT = self.fromT, self.toT
        res = []
        orig_points = Dist.points

        def spy(me, a, c, cb, phys):
            if me.calc == "GreatCircle":
                if cb is None:
                    return False
                res.append(gc_positions(BODIES[cb]["radius"], phys[cb]["pos"], a, c))
            else:
                res.append(vdist(a, c))
            return orig_points(me, a, c, cb, phys)

        Dist.points = spy
        try:
            probe.test(w)
        except KeyError:
            pass
        finally:
            Dist.points = orig_points
            self.fromKV, self.toKV, self.facility, self.lsite, self.label = saved
        return res[0] if res else None

    # ---- RunValidation -----------------------------------------------------------------------------------------------------------
    def validate(self, roster, editor_nodes):
        out = []
        if self.fromT == "Kerbal" and self.toT == "Kerbal" and self.fromK.name() == self.toK.name():
            out.append(["fail", fmt("#autoLOC_8004219")])
        if self.fromT == "Vessel" and self.toT == "Vessel" and self.fromV == self.toV:
            out.append(["fail", fmt("#autoLOC_8004220")])
        if self.fromT == "Kerbal":
            if self.fromK.kerbal is not None and roster[self.fromK.kerbal[0]] != "Assigned":
                out.append(["warn", fmt("#autoLOC_8002033", self.fromK.name())])
        if self.toT == "Kerbal":
            if self.toK.kerbal is not None and roster[self.toK.kerbal[0]] != "Assigned":
                out.append(["warn", fmt("#autoLOC_8002033", self.toK.name())])
        if self.toT == "Asteroid" and self.ast == 0:
            out.append(["fail", fmt("#autoLOC_8004223")])
        if self.toT == "Comet" and self.comet == 0:
            out.append(["fail", fmt("#autoLOC_8005448")])
        if self.toT == "Flag" and self.flag == 0:
            out.append(["fail", fmt("#autoLOC_8004224")])
        if self.toT == "NodeLabelNode" and self.node not in editor_nodes:
            out.append(["fail", fmt("#autoLOC_8004225")])
        return out

    # ---- GetNodeBodyParameterString ----------------------------------------------------------------------------------------------
    def vessel_text(self, vid):
        """GetVesselSituationByVesselID(vid), else UpdateFromMappedVesselIDs(vid): -> (flag, vesselName, situation)"""
        sit = self.m.vessel_by_id(vid)
        if sit is not None:
            return False, sit["name"], sit
        return self.m.update_from_mapped(vid)

    def body_text(self, f):
        name = f["name"]
        if name == "distanceFromTarget":
            text = self.base_body(f) + "\n"
            if self.fromT == "Vessel":
                flag, vname, sit = self.vessel_text(self.fromV)
                text2 = vname if (sit is not None or flag) else (fmt("#autoLOC_8100159") if self.fromV != 0 else fmt("#autoLOC_8004217"))
                text3 = text + fmt("#autoLOC_8004190", fmt("#autoLOC_8000001"), text2)
                if flag and sit is not None:
                    text3 = text3 + "\n" + fmt("#autoLOC_6002491", sit["name"]) + "\n"
                return text3
            return text + self.fromK.text()
        if name == "distanceToTarget":
            text4 = self.base_body(f) + "\n"
            t = self.toT
            if t == "Asteroid":
                a = self.m.asteroid(self.ast)
                text = a["name"] if a is not None else fmt("#autoLOC_6003000")
                return text4 + fmt("#autoLOC_8004190", fmt("#autoLOC_8000046"), text)
            if t == "Comet":
                a = self.m.comet(self.comet)
                text = fmt(a["name"]) if a is not None else fmt("#autoLOC_6003000")
                return text4 + fmt("#autoLOC_8004190", fmt("#autoLOC_6005065"), text)
            if t == "CelestialBody":
                disp = fmt("#autoLOC_8000273") if self.mcb is None else strip_gender_local(BODIES[self.mcb]["display"])
                return text4 + fmt("#autoLOC_8000267", disp)
            if t == "Flag":
                a = self.m.flag(self.flag)
                text = a["site"] if a is not None else fmt("#autoLOC_6003000")
                return text4 + fmt("#autoLOC_8004190", fmt("#autoLoc_6002179"), text)
            if t == "Kerbal":
                return text4 + self.toK.text()
            if t == "LaunchSite":
                return text4 + fmt("#autoLOC_8004190", fmt("#autoLOC_8000067"), "[" + self.site + "]")   # the synthetic GetLaunchSiteDisplayName
            if t == "Location":
                return text4 + self.loc.text()
            if t == "NodeLabelNode":
                text = fmt("#autoLOC_6003000") if self.node not in self.m.nodes else self.m.nodes[self.node]
                return text4 + fmt("#autoLOC_8004190", fmt("#autoLOC_8004199"), text)
            if t == "Vessel":
                sit2 = self.m.vessel_by_id(self.toV)
                found, vname2 = False, ""
                if sit2 is not None:
                    vname2 = sit2["name"]
                else:
                    found, vname2, sit2 = self.m.update_from_mapped(self.fromV)    # sic: distanceFromVesselID
                text5 = vname2 if (sit2 is not None or found) else fmt("#autoLOC_8100159")
                text6 = text4 + fmt("#autoLOC_8004190", fmt("#autoLOC_8000001"), text5)
                if found and sit2 is not None:
                    text6 = text6 + fmt("#autoLOC_6002491", sit2["name"]) + "\n"
                return text6
        if name == "distance":
            return fmt("#autoLOC_8004226", "<" if self.cmp == "LessThan" else ">", g15(self.distance))
        if name == "calculationType":
            return self.base_body(f)
        return ""

    # ---- UI visibility callbacks -------------------------------------------------------------------------------------------------
    @staticmethod
    def ui_from(sel):
        log = []
        if sel is not None:
            if sel == 1:      # Vessel
                log += [["fromKerbal", False], ["fromVessel", True]]
            elif sel == 0:    # Kerbal
                log += [["fromKerbal", True], ["fromVessel", False]]
        return log + [["body"]]      # UpdateNodeBodyUI -> guiNode.DisplayNodeBodyParameters

    @staticmethod
    def ui_to(sel):
        log = [[n, False] for n in ("toKerbal", "toVessel", "toLocation", "toBody", "toAsteroid", "toComet", "toFlag", "toLaunchSite", "toNode")]
        names = ["toAsteroid", "toComet", "toBody", "toFlag", "toKerbal", "toLaunchSite", "toLocation", "toNode", "toVessel"]
        if sel is not None and 0 <= sel < len(names):
            log.append([names[sel], True])
        return log + [["body"]]

    def score_mod(self, w, module):
        if module == "ScoreModule_Resource":
            return {"vessel": vtext(w, active(w)), "number": None}
        return {"vessel": None, "number": None}


def strip_gender_local(s):
    return s.split("^")[0] if "^" in s else s


def g15(x):
    """double.ToString() ('G'): 15 significant digits, no exponent for the magnitudes the scenarios use"""
    t = "%.15g" % x
    return t


# ---- scenario generation -------------------------------------------------------------------------------------------------------------
POS_POOL = [0.0, 1000.0, -2500.5, 75000.25, 600000.0, -1200000.0, 4000000.0]
ANGLE_POOL = [0.0, 0.3, -0.7, 1.1]
BODY_POS_POOL = [0.0, 5000000.0, -13599840256.0, 20000000.0]


def gen_phys(r, w):
    w["phys"] = [{"pos": [rng_pick(r, BODY_POS_POOL) for _ in range(3)], "frame": [rng_pick(r, ANGLE_POOL) for _ in range(3)]} for _ in BODIES]
    for v in w["vessels"]:
        v["pos"] = [rng_pick(r, POS_POOL) + r.randrange(0, 4) * 11.25 for _ in range(3)]
    w["sites"] = {}
    for s in SITE_NAMES:
        if r.random() < 0.5:
            w["sites"]["facility:" + s] = [rng_pick(r, POS_POOL) + 3.0 for _ in range(3)]
        if r.random() < 0.5:
            w["sites"]["launchsite:" + s] = [rng_pick(r, POS_POOL) - 7.5 for _ in range(3)]


def gen_mission(r):
    m = MissionModel()
    n = r.randrange(0, 3)
    for i in range(n):
        m.vessels.append({"pid": 100 + 7 * i, "name": rng_pick(r, ["Alpha", "Bravo", "#autoLOC_8100086"]), "craft": "", "node": i})
    # mapped vessels: (part, orig, cur, pvn, craft, sit, mappedId)
    for i in range(r.randrange(0, 3)):
        m.mapped.append(MappedVessel(5000 + i, 100 + 7 * i, rng_pick(r, [100, 107, 114, 999]), rng_pick(r, ["Booster", "Lander"]), "c%d.craft" % i, "", 700 + i))
    for i in range(r.randrange(0, 2)):
        m.flags.append({"pid": 300 + i, "site": rng_pick(r, ["Flag Site", "Tranquility"])})
        m.asteroids.append({"pid": 400 + i, "name": rng_pick(r, ["Ast-1", "Rock"])})
        m.comets.append({"pid": 500 + i, "name": rng_pick(r, ["Comet-A", "Halley"])})
    # node 98 is the subject node of the fixture; extra nodes 61 (label) and 62 (no label)
    m.nodes[guid_of(98)] = "Subject"
    m.nodes[guid_of(61)] = "Labelled"
    m.nodes[guid_of(62)] = "Plain"
    area = Area()
    area.body = rng_pick(r, [0, 1, 2])
    area.lat, area.lon, area.alt = rng_pick(r, [0.0, 12.5, -33.25]), rng_pick(r, [0.0, 45.0, 170.5]), rng_pick(r, [0.0, 250.0])
    area.radius = 2000.0

    def label(phys, area=area):
        return v3_to_f32(world_surface_position(BODIES[area.body], phys[area.body], area.lat, area.lon, area.alt))

    m.node_labels[guid_of(61)] = label
    m.node_labels[guid_of(62)] = False
    spec = {"nodes": [{"guid": guid_of(61), "title": "Labelled", "test": "TestGoTo", "area": {"body": area.body, "lat": area.lat, "lon": area.lon, "alt": area.alt, "radius": area.radius}},
                      {"guid": guid_of(62), "title": "Plain", "test": "TestEmptyTrue"}],
            "vessels": m.vessels, "mapped": [mm.to_json() for mm in m.mapped], "flags": m.flags, "asteroids": m.asteroids, "comets": m.comets}
    return m, spec


def gen_cfg(r):
    cfg = Cfg("N")
    keys = ["distanceFromTarget", "distanceFromVesselID", "distanceToTarget", "distanceToVesselID", "distanceToAsteroidID", "distanceToCometID", "distanceToFlagID",
            "distanceToLaunchSiteName", "distanceToNode", "calculationType", "comparisonOperator", "bodyName"]
    for k in keys:
        if r.random() < 0.8:
            if k == "distanceFromTarget":
                cfg.values.append((k, rng_pick(r, FROM_CHOICES + ["Bogus"])))
            elif k == "distanceToTarget":
                cfg.values.append((k, rng_pick(r, TO_CHOICES + ["Bogus"])))
            elif k in ("distanceFromVesselID", "distanceToVesselID"):
                cfg.values.append((k, rng_pick(r, ["0", "107", "100", "114", "999", "x"])))
            elif k == "distanceToAsteroidID":
                cfg.values.append((k, rng_pick(r, ["0", "400", "401"])))
            elif k == "distanceToCometID":
                cfg.values.append((k, rng_pick(r, ["0", "500", "501"])))
            elif k == "distanceToFlagID":
                cfg.values.append((k, rng_pick(r, ["0", "300", "301"])))
            elif k == "distanceToLaunchSiteName":
                cfg.values.append((k, rng_pick(r, SITE_NAMES)))
            elif k == "distanceToNode":
                cfg.values.append((k, rng_pick(r, GUIDS + ["not-a-guid"])))
            elif k == "calculationType":
                cfg.values.append((k, rng_pick(r, CALC + ["x"])))
            elif k == "comparisonOperator":
                cfg.values.append((k, rng_pick(r, LG + ["Equal"])))
            elif k == "bodyName":
                cfg.values.append((k, rng_pick(r, ["Kerbin", "Mun", "AnyValid", "", "Nowhere"])))
    for nm in ("DISTANCEFROMKERBAL", "DISTANCETOKERBAL"):
        if r.random() < 0.7:
            n = Cfg(nm)
            gen_mk_cfg(r, n)
            cfg.nodes.append(n)
    if r.random() < 0.8:
        n = Cfg("DISTANCETOLOCATION")
        for k in r.sample(["lon", "lat", "alt", "bodyName", "gizmoIcon"], r.randrange(0, 5)):
            n.values.append((k, rng_pick(r, ["Kerbin", "Mun", "Minmus"]) if k == "bodyName" else ("Flag" if k == "gizmoIcon" else rng_pick(r, ["0", "12.5", "-33.25", "45"]))))
        cfg.nodes.append(n)
    if r.random() < 0.8:
        cfg.values.append(("distance", rng_pick(r, ["0", "1000", "25000.5", "7500000", "bad"])))
    if r.random() < 0.3:
        cfg.values.append(("title", "Custom"))
    return cfg


FIELDS = [{"name": "distanceFromTarget", "guiName": "#autoLOC_8004196"}, {"name": "distanceToTarget", "guiName": "#autoLOC_8004200"},
          {"name": "distance", "guiName": "#autoLOC_8100017"}, {"name": "calculationType", "guiName": "#autoLOC_8004209"},
          {"name": "comparisonOperator", "guiName": "#autoLOC_8000052"}]


def body_ops_d(m):
    ops = []
    for f in FIELDS:
        f = dict(f)
        f["value"] = "x"
        f["has"] = True
        ops.append({"op": "body", "field": f, "out": m.body_text(f)})
    return ops


def bias_pids(r, m, w):
    """give the world vessels the persistent ids the configuration refers to (otherwise most targets would not exist in a world)"""
    wanted = [i for i in (m.fromV, m.toV, m.ast, m.comet, m.flag) if i != 0]
    vs = w["vessels"]
    if not vs or not wanted or r.random() < 0.15:
        return
    for i, v in enumerate(vs):
        if r.random() < 0.7:
            v["pid"] = wanted[(i + r.randrange(len(wanted))) % len(wanted)]
    # unique ids (FlightGlobals.PersistentVesselIds is a dictionary)
    seen = set()
    for v in vs:
        while v["pid"] in seen:
            v["pid"] += 1
        seen.add(v["pid"])
    if r.random() < 0.8:
        w["active"] = r.randrange(0, len(vs))


def margin_ok(m, w):
    d = m.measure(w)
    if d is None:
        return True
    if d != d:
        return False
    return abs(d - m.distance) > 1e-6 * max(1.0, abs(d))


def gen_case(r):
    mission, spec = gen_mission(r)
    m = Dist(mission)
    cfg = gen_cfg(r)
    load_throws = False
    try:
        m.load(cfg)
    except KeyError:
        load_throws = True
    if load_throws:
        return {"kind": "distance", "cls": "TestDistance", "cfg": cfg.to_json(), "mission": spec, "ops": [], "loadThrows": True, "tick": TICK}
    ops = [{"op": "mission", "spec": spec}] + head_ops(m) + body_ops_d(m)
    roster = {n: "Available" for n in KERBALS + ROSTER_TOURISTS}
    ops.append({"op": "validate", "out": m.validate(roster, set(mission.nodes.keys()))})
    # visibility callbacks
    for sel in (None, 0, 1):
        ops.append({"op": "uiFrom", "sel": sel, "out": Dist.ui_from(sel)})
    for sel in (None, 0, 2, 5, 8):
        ops.append({"op": "uiTo", "sel": sel, "out": Dist.ui_to(sel)})
    ops.append({"op": "uiSetup", "from": 1, "to": 6, "out": Dist.ui_from(1) + Dist.ui_to(6)})
    for step in range(5):
        for attempt in range(40):
            w = gen_world(r)
            bias_pids(r, m, w)
            gen_phys(r, w)
            # snapshot the state the step would mutate (caches) to retry cleanly
            if margin_ok(m, w):
                break
        else:
            continue
        ut = 100.0
        new_env(w, ops, ut)
        ops.append({"op": "phys", "phys": w["phys"], "vessels": {str(v["pid"]): v["pos"] for v in w["vessels"]}, "sites": w["sites"]})
        if r.random() < 0.4:
            ops.append({"op": "roster", "name": rng_pick(r, KERBALS), "status": rng_pick(r, ["Available", "Assigned"])})
            roster_name = ops[-1]["name"]
            roster[roster_name] = ops[-1]["status"]
            ops.append({"op": "validate", "out": m.validate(roster, set(mission.nodes.keys()))})
        try:
            res = m.test(w)
            ops.append({"op": "test", "out": res})
        except KeyError:
            ops.append({"op": "test", "out": "throws"})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": m.score_mod(w, "ScoreModule_Resource")})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Time", "out": m.score_mod(w, "ScoreModule_Time")})
        if r.random() < 0.3:
            old = rng_pick(r, [100, 107, 114, 400, 500, 300])
            new = old + 1000
            m.vessel_id_changed(old, new)
            ops.append({"op": "call", "fn": "OnVesselPersistentIdChanged", "a": [old, new]})
            ops.append({"op": "saved", "out": m.save().dump()})
            ops += body_ops_d(m)
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "distance", "cls": "TestDistance", "cfg": cfg.to_json(), "mission": spec, "ops": ops, "tick": TICK, "roster": True}


def generate(r):
    return [gen_case(r) for _ in range(320)]
