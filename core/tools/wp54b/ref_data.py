"""WP-54 slice 2 reference, part 1: the data classes (Expansions/Missions/{VesselGroundLocation,SurfaceArea,SurfaceVolume,Asteroid,Comet,MissionKerbal,
ParamChoices_*,LaunchSiteSituation,MissionOrbit,VesselLocation,Crew,MissionCraft}.cs, Editor/MissionBiome.cs). No kspcore code."""
import math
import random

from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, f32, fnum, dnum, fmt, tag, b, g9, body, BODIES, HOME, INF, soi, strip_gender, custom_0hh, custom_0, nfmt, KSPRandom, deg_vec

DEG = math.pi / 180.0
EPS_F = 1.401298464324817e-45


def rng_pick(r, seq):
    return seq[r.randrange(len(seq))]


def mathf_approx(a, c):
    a, c = f32(a), f32(c)
    return abs(f32(c - a)) < max(f32(1e-06 * max(abs(a), abs(c))), f32(EPS_F * 8.0))


def great_circle(R, lat1, lon1, lat2, lon2):
    def n(lat, lon):
        la, lo = lat * DEG, lon * DEG
        return (math.cos(la) * math.cos(lo), math.cos(la) * math.sin(lo), math.sin(la))
    a, c = n(lat1, lon1), n(lat2, lon2)
    d = a[0] * c[0] + a[1] * c[1] + a[2] * c[2]
    return R * math.acos(max(-1.0, min(1.0, d)))


def rel_pos(bd, lat, lon, alt):
    la, lo = lat * DEG, lon * DEG
    n = (math.cos(la) * math.cos(lo), math.cos(la) * math.sin(lo), math.sin(la))
    return tuple(c * (bd["radius"] + alt) for c in n)


def parse_f(text):
    return f32(float(text))


def uint_try(text):
    t = text.strip()
    return int(t) if t.isdigit() and int(t) < 2 ** 32 else 0


def dbl(x):
    return dnum(x)


def _net_exp(t):
    """C printf exponent 'e-05' -> .NET 'E-05' (two digit minimum, explicit sign)"""
    if "e" not in t:
        return t
    mant, exp = t.split("e")
    return mant + "E" + exp[0] + exp[1:].lstrip("0").zfill(2)


def D(x):
    """double.ToString("G17") (ConfigNode.AddValue(double)); positional values only"""
    return _net_exp("%.17g" % x)


def F(x):
    """float.ToString("G9") (ConfigNode.AddValue(float))"""
    return _net_exp("%.9g" % f32(x))


def FS(x):
    """float.ToString() (string concatenation / Localizer arguments)"""
    return fnum(f32(x))


GIZMOS = ["Rocket", "AirCraft", "Asteroid", "Kerbal", "LaunchSite", "Flag", "Comet"]
POOL_NUM = ["0", "12.5", "-45.5", "74.5", "90", "-74.5625", "0.25", "33.25", "-10", "150.75"]
POOL_ROT = ["0,0,0", "10,20,30", "90,0,180.5", "0,45.5,0", "270,10,1.25"]


# ---- VesselGroundLocation ----------------------------------------------------------------------------------------------------------
class VGL:
    def __init__(self):
        self.icon = "Flag"
        self.body = None
        self.splashed = False
        self.lat = self.lon = self.alt = 0.0
        self.rot = [0.0, 0.0, 0.0]

    def load(self, cfg):
        for k, v in cfg.values:
            if k == "lon":
                self.lon = float(v)
            elif k == "gizmoIcon":
                assert v in GIZMOS
                self.icon = v
            elif k == "alt":
                self.alt = float(v)
            elif k == "lat":
                self.lat = float(v)
            elif k == "rot":
                p = v.split(",")
                self.rot = [parse_f(c) for c in p[:3]] if len(p) >= 3 else [0.0, 0.0, 0.0]
            elif k == "splashed":
                self.splashed = v.strip().lower() == "true"
            elif k == "bodyName":
                if v != "":
                    self.body = body_by_name_or_none(v)
        if self.body is None:
            self.body = HOME

    def save(self, c):
        c.values.append(("bodyName", "" if self.body is None else body(self.body)["name"]))
        c.values.append(("gizmoIcon", self.icon))
        c.values.append(("lat", D(self.lat)))
        c.values.append(("lon", D(self.lon)))
        c.values.append(("alt", D(self.alt)))
        c.values.append(("rot", deg_vec(self.rot)))
        c.values.append(("splashed", b(self.splashed)))

    def text(self):
        return fmt("#autoLOC_8004191", body(self.body)["display"], custom_0hh(self.lon), custom_0hh(self.lat))


def body_by_name_or_none(n):
    for i, bd in enumerate(BODIES):
        if bd["name"] == n:
            return i
    return None


def gen_ground(r):
    cfg = Cfg("GROUNDPOINT")
    keys = ["lon", "gizmoIcon", "alt", "lat", "rot", "splashed", "bodyName"]
    r.shuffle(keys)
    for k in keys[: r.randrange(0, len(keys) + 1)]:
        if k in ("lon", "alt", "lat"):
            cfg.values.append((k, rng_pick(r, POOL_NUM)))
        elif k == "gizmoIcon":
            cfg.values.append((k, rng_pick(r, GIZMOS)))
        elif k == "rot":
            cfg.values.append((k, rng_pick(r, POOL_ROT)))
        elif k == "splashed":
            cfg.values.append((k, rng_pick(r, ["True", "False"])))
        else:
            cfg.values.append((k, rng_pick(r, ["Kerbin", "Mun", "Minmus", "", "Nowhere"])))
    g = VGL()
    g.load(cfg)
    out = Cfg("N")
    g.save(out)
    wp = {"celestialName": body(g.body)["name"], "latitude": g.lat, "longitude": g.lon, "caption1": fmt("#autoLOC_8002000")}
    return {"kind": "ground", "cfg": cfg.to_json(), "save": out.dump(), "text": g.text(), "waypoint": wp, "extra": fmt("#autoLOC_8002000")}


# ---- SurfaceArea ---------------------------------------------------------------------------------------------------------------------
class Area:
    def __init__(self):
        self.body = HOME
        self.lat = self.lon = self.alt = 0.0
        self.radius = 0.0

    def load(self, cfg):
        for k, v in cfg.values:
            if k == "radius":
                self.radius = parse_f(v)
            elif k == "alt":
                self.alt = float(v)
            elif k == "lon":
                self.lon = float(v)
            elif k == "lat":
                self.lat = float(v)
            elif k == "bodyName":
                if v != "":
                    self.body = body_by_name_or_none(v)
        if self.body is None:
            self.body = HOME

    def save(self, c):
        c.values.append(("bodyName", "" if self.body is None else body(self.body)["name"]))
        c.values.append(("lat", D(self.lat)))
        c.values.append(("lon", D(self.lon)))
        c.values.append(("alt", D(self.alt)))
        c.values.append(("radius", F(self.radius)))

    def text(self):
        return fmt("#autoLOC_8007312", body(self.body)["display"], custom_0hh(self.lon), custom_0hh(self.lat), custom_0hh(f32(self.radius / f32(1000.0))))

    def equals(self, o):
        return (body(self.body)["name"] == body(o.body)["name"] and self.lat == o.lat and self.lon == o.lon and mathf_approx(self.radius, o.radius))

    def dist(self, lat, lon):
        return great_circle(body(self.body)["radius"], lat, lon, self.lat, self.lon)


def gen_area(r):
    cfg = Cfg("N")
    for k in r.sample(["radius", "alt", "lon", "lat", "bodyName"], r.randrange(0, 6)):
        if k == "radius":
            cfg.values.append((k, rng_pick(r, ["150000", "5000.5", "100000", "600000", "250.25"])))
        elif k == "bodyName":
            cfg.values.append((k, rng_pick(r, ["Kerbin", "Mun", "Minmus", "", "Nowhere"])))
        else:
            cfg.values.append((k, rng_pick(r, POOL_NUM)))
    a = Area()
    a.load(cfg)
    out = Cfg("N")
    a.save(out)
    pts = []
    for _ in range(6 if a.radius > 0 else 0):
        lat, lon = r.uniform(-80, 80), r.uniform(-170, 170)
        if r.random() < 0.5:  # near the centre (inside) / far
            lat = max(-85.0, min(85.0, a.lat + r.uniform(-1.0, 1.0)))
            lon = a.lon + r.uniform(-1.0, 1.0)
        d = a.dist(lat, lon)
        if a.radius > 0 and abs(d - a.radius) < 1e-4 * max(a.radius, 1.0):
            continue
        acc = 1.0 - max(0.0, min(1.0, d / a.radius)) if a.radius > 0 else None
        pts.append({"lat": lat, "lon": lon, "inside": d <= a.radius, "accuracy": acc})
    clone = Area()
    clone.load(cfg)
    other = Area()
    other.load(cfg)
    other.radius = f32(other.radius * 1.5 + 1.0)
    wp = {"celestialName": body(a.body)["name"], "latitude": a.lat, "longitude": a.lon, "altitude": a.alt, "radius": f32(a.radius),
          "caption1": fmt("#autoLOC_8000274", FS(a.radius))}
    return {"kind": "area", "cfg": cfg.to_json(), "save": out.dump(), "text": a.text(), "extra": fmt("#autoLOC_8000274", FS(a.radius)), "waypoint": wp, "points": pts,
            "equalsClone": a.equals(clone), "equalsOther": a.equals(other)}


# ---- SurfaceVolume -------------------------------------------------------------------------------------------------------------------
class Volume:
    def __init__(self, default=True):
        self.body = HOME
        self.lat = self.lon = self.alt = 0.0
        self.shape = "Cone"
        self.radius = self.hs = self.hmin = self.hmax = 0.0
        self.show = False
        if default:
            R = body(HOME)["radius"]
            self.hs = 0.0
            self.radius = f32(f32(R) / 4.0)
            self.hmax = f32(f32(R) * 0.25)
            self.hmin = 0.0

    def load(self, cfg):
        f1 = f2 = f3 = False
        for k, v in cfg.values:
            if k == "hConeMax":
                f3 = True
                self.hmax = parse_f(v)
            elif k == "alt":
                self.alt = float(v)
            elif k == "hConeMin":
                f2 = True
                self.hmin = parse_f(v)
            elif k == "hSphere":
                f1 = True
                self.hs = parse_f(v)
            elif k == "lat":
                self.lat = float(v)
            elif k == "lon":
                self.lon = float(v)
            elif k == "shape":
                assert v in ("Cone", "Sphere")
                self.shape = v
            elif k == "targetBody":
                if v != "":
                    self.body = body_by_name_or_none(v)
            elif k == "rad":
                self.radius = parse_f(v)
        if self.shape == "Sphere":
            if f1:
                self.alt = float(self.hs)
        elif f2 and f3:
            self.alt = float(f32((self.hmin + self.hmax) / 2.0))
        if self.body is None:
            self.body = HOME

    def save(self, c):
        c.values.append(("targetBody", "" if self.body is None else body(self.body)["name"]))
        c.values.append(("lat", D(self.lat)))
        c.values.append(("lon", D(self.lon)))
        c.values.append(("alt", D(self.alt)))
        c.values.append(("rad", F(self.radius)))
        c.values.append(("shape", self.shape))
        c.values.append(("hSphere", F(self.hs)))
        c.values.append(("hConeMin", F(self.hmin)))
        c.values.append(("hConeMax", F(self.hmax)))

    def text(self):
        bd = body(self.body)
        k = lambda x: custom_0hh(f32(x / f32(1000.0)))
        if self.shape == "Sphere":
            return fmt("#autoLOC_8007313", bd["display"], custom_0hh(self.lon), custom_0hh(self.lat), k(self.radius), k(self.hs))
        return fmt("#autoLOC_8007314", bd["display"], custom_0hh(self.lon), custom_0hh(self.lat), k(self.radius), k(self.hmin), k(self.hmax))

    def waypoint(self):
        bd = body(self.body)
        unit = fmt("#autoLOC_289929")
        c1 = fmt("#autoLOC_8004190", fmt("#autoLOC_8200035"), FS(self.radius) + unit)
        if self.shape == "Cone":
            c2 = fmt("#autoLOC_8004190", fmt("#autoLOC_8200036"), FS(self.hmin)) + unit
            c3 = fmt("#autoLOC_8004190", fmt("#autoLOC_8200037"), FS(self.hmax)) + unit
        else:
            c2 = fmt("#autoLOC_8004190", fmt("#autoLOC_8200038"), FS(self.hs)) + unit
            c3 = ""
        return {"celestialName": bd["name"], "latitude": self.lat, "longitude": self.lon, "altitude": self.alt, "radius": f32(self.radius),
                "caption1": c1, "caption2": c2, "caption3": c3}

    def inside(self, lat, lon, alt):
        bd = body(self.body)
        lat, lon, alt = f32(lat), f32(lon), f32(alt)
        if self.shape == "Sphere":
            p = rel_pos(bd, lat, lon, alt)
            q = rel_pos(bd, self.lat, self.lon, self.hs)
            return sum((p[i] - q[i]) ** 2 for i in range(3)) < f32(self.radius) ** 2
        if alt < self.hmax and alt > self.hmin:
            return great_circle(bd["radius"], lat, lon, self.lat, self.lon) <= f32(self.radius)
        return False

    def margin_ok(self, lat, lon, alt):
        """true when the answer is far from every boundary (libm differences cannot flip it)"""
        bd = body(self.body)
        lat, lon, alt = f32(lat), f32(lon), f32(alt)
        if self.shape == "Sphere":
            p = rel_pos(bd, lat, lon, alt)
            q = rel_pos(bd, self.lat, self.lon, self.hs)
            d = math.sqrt(sum((p[i] - q[i]) ** 2 for i in range(3)))
            return abs(d - f32(self.radius)) > 1e-4 * max(1.0, f32(self.radius))
        if abs(alt - self.hmax) < 1e-3 * max(1.0, abs(self.hmax)) or abs(alt - self.hmin) < 1e-3 * max(1.0, abs(self.hmin)):
            return False
        if self.hmin < alt < self.hmax:
            d = great_circle(bd["radius"], lat, lon, self.lat, self.lon)
            return abs(d - f32(self.radius)) > 1e-4 * max(1.0, f32(self.radius))
        return True


def gen_volume(r):
    cfg = Cfg("N")
    for k in r.sample(["hConeMax", "alt", "hConeMin", "hSphere", "lat", "lon", "shape", "targetBody", "rad"], r.randrange(0, 10)):
        if k in ("hConeMax", "hConeMin", "hSphere", "rad"):
            cfg.values.append((k, rng_pick(r, ["150000", "5000.5", "100000", "0", "250.25", "30000"])))
        elif k == "shape":
            cfg.values.append((k, rng_pick(r, ["Cone", "Sphere"])))
        elif k == "targetBody":
            cfg.values.append((k, rng_pick(r, ["Kerbin", "Mun", "", "Nowhere"])))
        else:
            cfg.values.append((k, rng_pick(r, POOL_NUM)))
    v = Volume()
    v.load(cfg)
    out = Cfg("N")
    v.save(out)
    pts = []
    for _ in range(8):
        lat, lon = v.lat + r.uniform(-2.0, 2.0), v.lon + r.uniform(-2.0, 2.0)
        lat = max(-85.0, min(85.0, lat))
        alt = r.choice([0.0, f32(v.hmin + 1.0), f32(v.hmax - 1.0), f32(v.hmax + 5000.0), f32(v.hs), f32(v.hs + 1000.0), 2500.0])
        if v.margin_ok(lat, lon, alt):
            pts.append({"lat": f32(lat), "lon": f32(lon), "alt": alt, "inside": v.inside(lat, lon, alt)})
    clone = Volume()
    clone.load(cfg)
    other = Volume()
    other.load(cfg)
    other.hmax = f32(other.hmax + 100.0)
    return {"kind": "volume", "cfg": cfg.to_json(), "save": out.dump(), "text": v.text(), "extra": fmt("#autoLOC_8000274", FS(v.radius)), "waypoint": v.waypoint(),
            "points": pts, "hasLabel": [False, True], "default": {"radius": f32(Volume().radius), "hmax": f32(Volume().hmax)}, "labelDefault": False,
            "equalsClone": (v.body == clone.body and v.lat == clone.lat and v.lon == clone.lon and mathf_approx(v.radius, clone.radius) and v.shape == clone.shape and mathf_approx(v.hs, clone.hs) and mathf_approx(v.hmin, clone.hmin) and mathf_approx(v.hmax, clone.hmax)),
            "equalsOther": False}


# ---- Asteroid / Comet --------------------------------------------------------------------------------------------------------------
CLASSES = ["A", "B", "C", "D", "E", "F", "G", "H", "I"]
COMET_DEFS = [("Short", "Short-period"), ("Long", "Long-period"), ("Halley", "Halley-type")]


def gen_asteroid(r):
    cfg = Cfg("N")
    st = {"name": "Asteroid", "seed": 0, "type": "Regular", "cls": "C", "pid": 0}
    for k in r.sample(["persistentId", "asteroidType", "asteroidClass", "asteroidSeed", "asteroidName"], r.randrange(0, 6)):
        if k == "persistentId":
            v = rng_pick(r, ["0", "12345", "4294967295", "abc", "-5"])
            cfg.values.append((k, v))
            st["pid"] = uint_try(v)
        elif k == "asteroidType":
            v = rng_pick(r, ["Regular", "Glimmeroid"])
            cfg.values.append((k, v))
            st["type"] = v
        elif k == "asteroidClass":
            v = rng_pick(r, CLASSES)
            cfg.values.append((k, v))
            st["cls"] = v
        elif k == "asteroidSeed":
            v = rng_pick(r, ["0", "987654", "2147483647", "zz"])
            cfg.values.append((k, v))
            st["seed"] = uint_try(v)
        else:
            v = rng_pick(r, ["Rock 1", "AB-12", "Potato"])
            cfg.values.append((k, v))
            st["name"] = v
    out = Cfg("N")
    out.values += [("asteroidName", st["name"]), ("asteroidSeed", str(st["seed"])), ("asteroidClass", st["cls"]), ("asteroidType", st["type"]), ("persistentId", str(st["pid"]))]
    return {"kind": "asteroid", "cfg": cfg.to_json(), "save": out.dump()}


def gen_comet(r):
    defs = r.choice([None, COMET_DEFS, COMET_DEFS[1:]])
    cfg = Cfg("N")
    st = {"name": "Comet", "seed": 0, "radius": 0.0, "type": "" if not defs else defs[0][0], "disp": "" if not defs else defs[0][1], "cls": "C", "pid": 0}
    for k in r.sample(["persistentId", "cometClass", "cometType", "cometradius", "cometSeed", "cometName"], r.randrange(0, 7)):
        if k == "persistentId":
            v = rng_pick(r, ["0", "777", "bad"])
            cfg.values.append((k, v))
            st["pid"] = uint_try(v)
        elif k == "cometClass":
            v = rng_pick(r, CLASSES)
            cfg.values.append((k, v))
            st["cls"] = v
        elif k == "cometType":
            v = rng_pick(r, ["Short", "Long", "Halley", "Other"])
            cfg.values.append((k, v))
            st["type"] = v
            if defs is not None:  # OnCometTypeChanged: cometDisplayType = cometType, then the first definition with that name
                st["disp"] = v
                for n, d in defs:
                    if n == v:
                        st["disp"] = d
                        break
        elif k == "cometradius":
            v = rng_pick(r, ["0", "1.5", "250.25", "x"])
            cfg.values.append((k, v))
            try:
                st["radius"] = parse_f(v)
            except ValueError:
                st["radius"] = 0.0
        elif k == "cometSeed":
            v = rng_pick(r, ["0", "1633779195", "42"])
            cfg.values.append((k, v))
            st["seed"] = uint_try(v)
        else:
            v = rng_pick(r, ["Halley", "Churyumov"])
            cfg.values.append((k, v))
            st["name"] = v
    out = Cfg("N")
    out.values += [("cometName", st["name"]), ("cometSeed", str(st["seed"])), ("cometradius", F(st["radius"])), ("cometType", st["type"]), ("cometClass", st["cls"]),
                   ("persistentId", str(st["pid"]))]
    return {"kind": "comet", "defs": None if defs is None else [list(d) for d in defs], "cfg": cfg.to_json(), "save": out.dump(), "display": st["disp"],
            "initial": {"type": "" if not defs else defs[0][0], "display": "" if not defs else defs[0][1]}}


# ---- MissionBiome / ParamChoices_CelestialBodySurface -------------------------------------------------------------------------------
def biome_name(r):
    return rng_pick(r, ["", "Shores", "Highlands", "Grasslands"])


def gen_surface(r):
    """ParamChoices_CelestialBodySurface: Load -> Save, NodeBodyParameterString, waypoint / label presence."""
    st = {"choice": "bodyData", "bodyName": None, "biome": (HOME, ""), "area": Area(), "site": "LaunchPad"}
    st["area"].body = HOME
    st["area"].radius = 150000.0
    # constructor of the Awake blocks: bodyData = MissionCelestialBody(null=any | home), biome = (home, ""), area = (home, 0, 0, 150000), site = ""
    anyBody = r.random() < 0.5
    st["body"] = None if anyBody else HOME
    st["site"] = ""
    cfg = Cfg("N")
    choices = ["anyData", "bodyData", "biomeData", "areaData", "launchSiteName"]
    if r.random() < 0.85:
        st["choice"] = rng_pick(r, choices)
        cfg.values.append(("locationChoice", st["choice"]))
    if r.random() < 0.6:
        bn = rng_pick(r, ["Kerbin", "Mun", "AnyValid", "Nowhere"])
        n = Cfg("BODYDATA")
        n.values.append(("bodyName", bn))
        cfg.nodes.append(n)
        if bn == "AnyValid":
            st["body"] = None
        elif bn != "":
            st["body"] = body_by_name_or_none(bn) if body_by_name_or_none(bn) is not None else None
    if r.random() < 0.6:
        n = Cfg("BIOMEDATA")
        bn = rng_pick(r, ["Kerbin", "Mun", ""])
        n.values.append(("bodyName", bn))
        bi = biome_name(r)
        if bi:
            n.values.append(("biomeName", bi))
        cfg.nodes.append(n)
        if bn != "":
            st["biome"] = (body_by_name_or_none(bn), st["biome"][1])
        if bi:
            st["biome"] = (st["biome"][0], bi)
    if r.random() < 0.6:
        n = Cfg("GROUNDAREA")
        for k in r.sample(["radius", "lon", "lat", "bodyName"], r.randrange(0, 5)):
            n.values.append((k, rng_pick(r, ["150000", "5000.5"]) if k == "radius" else (rng_pick(r, ["Kerbin", "Mun"]) if k == "bodyName" else rng_pick(r, POOL_NUM))))
        cfg.nodes.append(n)
        st["area"].load(n)
    if r.random() < 0.6:
        v = rng_pick(r, ["LaunchPad", "Runway", "Island"])
        cfg.values.append(("LAUNCHSITEDATA", v))
        st["site"] = v
    # Save
    out = Cfg("N")
    out.values.append(("locationChoice", st["choice"]))
    nb = Cfg("BODYDATA")
    nb.values.append(("bodyName", "AnyValid" if st["body"] is None else body(st["body"])["name"]))
    out.nodes.append(nb)
    ni = Cfg("BIOMEDATA")
    ni.values.append(("bodyName", body(st["biome"][0])["name"]))
    if st["biome"][1]:
        ni.values.append(("biomeName", st["biome"][1]))
    out.nodes.append(ni)
    na = Cfg("GROUNDAREA")
    st["area"].save(na)
    out.nodes.append(na)
    out.values.append(("LAUNCHSITEDATA", st["site"]))
    # texts
    c = st["choice"]
    if c == "anyData":
        text = fmt("#autoLOC_8000272")
    elif c == "bodyData":
        text = fmt("#autoLOC_8000267", fmt("#autoLOC_8000273") if st["body"] is None else strip_gender(body(st["body"])["display"]))
    elif c == "biomeData":
        bd = body(st["biome"][0])
        bt = fmt("#autoLOC_8002051") if not st["biome"][1] else st["biome"][1]  # GetBiomedisplayName: body without a BiomeMap -> the tag itself
        text = fmt("#autoLOC_8000268", bd["display"], bt)
    elif c == "areaData":
        text = st["area"].text()
    else:
        text = fmt("#autoLOC_8004190", fmt("#autoLOC_8000067"), st["site"])
    return {"kind": "surface", "bodyAny": anyBody, "cfg": cfg.to_json(), "save": out.dump(), "text": text, "hasWaypoint": c == "areaData",
            "extra": fmt("#autoLOC_8000274", FS(st["area"].radius)) if c == "areaData" else ""}


# ---- LaunchSiteSituation -----------------------------------------------------------------------------------------------------------
def sanitize(name):
    """KSPUtil.SanitizeString(name, '_', replaceEmpty: false) then Replace(' ', '_')"""
    out = "".join(c if (c.isalnum() or c in "_-.") else "_" for c in name)
    return out.replace(" ", "_")


def gen_launchsite(r):
    cfg = Cfg("N")
    st = {"name": fmt("#autoLOC_8100066"), "facility": "VAB", "ramp": False, "splashed": False}
    for k in r.sample(["splashed", "showRamp", "facility", "launchSiteName"], r.randrange(0, 5)):
        if k in ("splashed", "showRamp"):
            v = rng_pick(r, ["True", "False"])
            cfg.values.append((k, v))
            if k == "splashed":
                st["splashed"] = v == "True"
            else:
                st["ramp"] = v == "True"
        elif k == "facility":
            v = rng_pick(r, ["VAB", "SPH", "None"])
            cfg.values.append((k, v))
            st["facility"] = v
        else:
            v = rng_pick(r, ["My Site", "Island-2", "Pad 7"])
            cfg.values.append((k, v))
            st["name"] = v
    # the ground location of the constructor is random (KSPRandom seeded from the tick count): the GROUNDPOINT node overrides everything it sets
    g = Cfg("GROUNDPOINT")
    g.values += [("lon", "-74.5"), ("lat", "0.25"), ("bodyName", "Kerbin"), ("rot", "0,0,%s" % rng_pick(r, ["0", "10", "300", "275.5"])), ("alt", "0"),
                 ("splashed", "False"), ("gizmoIcon", "LaunchSite")]
    cfg.nodes.append(g)
    aligned = r.random() < 0.5
    if aligned:
        cfg.values.append(("alignedNorth", "True"))
    rz = parse_f(g.values[3][1].split(",")[2])
    if not aligned:
        rz = f32(rz + 90.0)
        if rz > 360.0:
            rz = f32(rz - 360.0)
    gl = VGL()
    gl.load(g)
    gl.rot[2] = rz
    out = Cfg("LAUNCHSITESITUATION")
    out.values += [("launchSiteName", st["name"]), ("facility", st["facility"]), ("showRamp", b(st["ramp"])), ("splashed", b(st["splashed"]))]
    gn = Cfg("GROUNDPOINT")
    gl.save(gn)
    out.nodes.append(gn)
    out.values.append(("alignedNorth", "True"))
    return {"kind": "launchsite", "cfg": cfg.to_json(), "save": out.dump(), "objectName": sanitize(st["name"]), "name": st["name"]}


# ---- MissionOrbit --------------------------------------------------------------------------------------------------------------------
class Orb:
    def __init__(self, ref=HOME, sma=None, ecc=0.0, inc=0.0001, lan=0.0, argpe=0.0, mna=0.0, eph=0.0):
        bd = body(ref)
        self.ref = ref
        self.sma = bd["radius"] + bd["radius"] * 0.25 if sma is None else sma
        self.ecc, self.inc, self.lan, self.argpe, self.mna, self.eph = ecc, inc, lan, argpe, mna, eph
        self.deg_mna = self.mna * (180.0 / math.pi)
        self.edit_rot = bd["rot"] - bd["drot"]
        self.cache = 0.0
        self.orb_ref, self.orb_sma, self.orb_ecc, self.orb_inc, self.orb_lan, self.orb_argpe, self.orb_mna, self.orb_eph = self.ref, self.sma, self.ecc, self.inc, self.lan, self.argpe, self.mna, self.eph

    @classmethod
    def from_cfg(cls, cfg):
        o = cls()
        o.ref = 0
        o.sma = o.ecc = o.inc = o.lan = o.argpe = o.mna = o.eph = 0.0
        for k, v in cfg.values:
            if k == "MNA":
                o.mna = float(v)
            elif k == "ECC":
                o.ecc = float(v)
            elif k == "REF":
                o.ref = int(v)
            elif k == "SMA":
                o.sma = float(v)
            elif k == "LPE":
                o.argpe = float(v)
            elif k == "LAN":
                o.lan = float(v)
            elif k == "EPH":
                o.eph = float(v)
            elif k == "INC":
                o.inc = float(v)
        o.deg_mna = o.mna * (180.0 / math.pi)
        o.edit_rot = body(o.ref)["rot"] - body(o.ref)["drot"]
        o.orb_ref, o.orb_sma, o.orb_ecc, o.orb_inc, o.orb_lan, o.orb_argpe, o.orb_mna, o.orb_eph = o.ref, o.sma, o.ecc, o.inc, o.lan, o.argpe, o.mna, o.eph
        return o

    def _sync(self):
        pass

    @property
    def bd(self):
        return body(self.orb_ref)

    def set_body(self, i):
        self.orb_ref = i
        self.ref = i

    def set_sma(self, v):
        bd = self.bd
        if (1.0 + self.ecc) * v - bd["radius"] <= soi(bd) and v >= bd["radius"] / 2.0:
            self.sma = v
            self.orb_sma = v

    def apoapsis(self):
        return (1.0 + self.ecc) * self.sma - self.bd["radius"]

    def periapsis(self):
        return (1.0 - self.ecc) * self.sma - self.bd["radius"]

    def max_radius(self):
        bd = self.bd
        if math.isinf(soi(bd)):
            return bd["radius"] * 10.0  # (no orbiting bodies in the synthetic world)
        return soi(bd) - 1.0

    def set_apo(self, value):
        if value < 0.0:
            value = 0.0
        mx = self.max_radius()
        pe = self.periapsis()
        if value < pe:
            return self.set_apo(pe)
        if value > mx:
            return self.set_ecc(0.9999)
        self.set_sma((value + self.periapsis()) / 2.0 + self.bd["radius"])
        self.set_ecc((value - self.sma + self.bd["radius"]) / self.sma)

    def set_peri(self, value):
        num = 0.0 - self.bd["radius"]
        ap = self.apoapsis()
        if value > ap:
            return self.set_peri(ap)
        if value <= num:
            return self.set_ecc(0.9999)
        self.set_sma((self.apoapsis() + value) / 2.0 + self.bd["radius"])
        self.set_ecc((self.sma - self.bd["radius"] - value) / self.sma)

    def set_ecc(self, v):
        if (1.0 + v) * self.sma - self.bd["radius"] <= soi(self.bd):
            self.ecc = v
            self.orb_ecc = v

    def set_inc(self, v):
        self.inc = v
        self.orb_inc = v

    def set_lan(self, v):
        self.lan = v
        self.orb_lan = v

    def set_argpe(self, v):
        self.argpe = v
        self.orb_argpe = v

    def set_epoch(self, v):
        self.eph = v
        self.orb_eph = v

    def set_mna(self, v):
        self.deg_mna = v
        self.mna = v * (math.pi / 180.0)
        self.orb_mna = v * (math.pi / 180.0)

    def reset(self):
        self.set_ecc(0.0)
        self.set_sma(self.bd["radius"] + 100000.0)
        self.set_inc(0.001)
        self.set_lan(0.0)
        self.set_argpe(0.0)
        self.set_mna(0.0)
        self.set_epoch(0.0)

    def load(self, cfg):
        for k, v in cfg.values:
            if k == "MNA":
                self.set_mna(float(v) * (180.0 / math.pi))
            elif k == "ECC":
                self.set_ecc(float(v))
            elif k == "editorRotation":
                self.edit_rot = float(v)
            elif k == "SMA":
                self.cache = float(v)
            elif k == "LAN":
                self.set_lan(float(v))
            elif k == "REF":
                self.set_body(int(v))
                self.set_sma(self.cache)
            elif k == "EPH":
                self.set_epoch(float(v))
            elif k == "INC":
                self.set_inc(float(v))
            elif k == "LPE":
                self.set_argpe(float(v))

    def save(self, c):
        c.values += [("SMA", D(self.sma)), ("ECC", D(self.ecc)), ("INC", D(self.inc)), ("LPE", D(self.argpe)), ("LAN", D(self.lan)), ("MNA", D(self.mna)),
                     ("EPH", D(self.eph)), ("REF", str(self.ref)), ("editorRotation", D(self.edit_rot))]

    def equals(self, o):
        return (self.ref == o.ref and self.sma == o.sma and self.ecc == o.ecc and self.inc == o.inc and self.lan == o.lan and self.argpe == o.argpe and self.eph == o.eph and self.mna == o.mna)

    def text(self):
        return fmt("#autoLOC_8004192", strip_gender(self.bd["display"]))

    def snapshot(self):
        return [self.sma, self.ecc, self.inc, self.lan, self.argpe, self.deg_mna, self.eph, self.ref, self.orb_sma, self.orb_ecc, self.orb_inc, self.orb_lan,
                self.orb_argpe, self.orb_mna, self.orb_eph, self.orb_ref]


ORB_POOL = {"SMA": ["900000", "1200000", "700000.5", "5000000"], "ECC": ["0", "0.25", "0.5", "0.75", "0.9999"], "INC": ["0", "28.5", "90", "0.25"],
            "LPE": ["0", "45.5", "180"], "LAN": ["0", "10.5", "270.25"], "MNA": ["0", "1.5", "3.0625"], "EPH": ["0", "100", "2500.5"]}


def gen_orbit(r):
    while True:
        try:
            res = _gen_orbit(r)
        except ZeroDivisionError:
            continue
        if all(math.isfinite(x) for sn in res['snapshots'] + [res['loadSnapshot']] for x in sn):
            return res


def _gen_orbit(r):
    cfg = Cfg("ORBIT")
    ref = r.randrange(0, len(BODIES))
    for k in r.sample(["SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH"], r.randrange(2, 8)):
        cfg.values.append((k, rng_pick(r, ORB_POOL[k])))
    cfg.values.append(("REF", str(ref)))
    o = Orb.from_cfg(cfg)
    ops = []
    snaps = [o.snapshot()]
    for _ in range(r.randrange(2, 7)):
        op = r.choice(["Apoapsis", "Periapsis", "SemiMajorAxis", "Eccentricity", "Inclination", "Lan", "ArgumentOfPeriapsis", "Epoch", "MeanAnomalyAtEpoch", "Reset", "Body"])
        if op == "Reset":
            o.reset()
            ops.append({"op": op})
        elif op == "Body":
            i = r.randrange(0, len(BODIES))
            o.set_body(i)
            ops.append({"op": op, "value": i})
        else:
            val = rng_pick(r, ["100000", "250000.5", "0.5", "10", "45.25", "-5000", "800000", "0.9999", "3.0625"])
            x = float(val)
            ops.append({"op": op, "value": val})
            {"Apoapsis": o.set_apo, "Periapsis": o.set_peri, "SemiMajorAxis": o.set_sma, "Eccentricity": o.set_ecc, "Inclination": o.set_inc, "Lan": o.set_lan,
             "ArgumentOfPeriapsis": o.set_argpe, "Epoch": o.set_epoch, "MeanAnomalyAtEpoch": o.set_mna}[op](x)
        snaps.append(o.snapshot())
    out = Cfg("N")
    o.save(out)
    # a second orbit, loaded with the setters (MissionOrbit.Load) from the saved node
    o2 = Orb.from_cfg(Cfg("ORBIT", [("SMA", "900000"), ("ECC", "0"), ("INC", "0"), ("LPE", "0"), ("LAN", "0"), ("MNA", "0"), ("EPH", "0"), ("REF", "0")]))
    lc = Cfg("ORBIT")
    for k in r.sample(["SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH", "editorRotation"], r.randrange(1, 9)):
        lc.values.append((k, rng_pick(r, ORB_POOL.get(k, ["1.5", "10"]))))
    lc.values.append(("REF", str(r.randrange(0, len(BODIES)))))
    o2.load(lc)
    out2 = Cfg("N")
    o2.save(out2)
    clone = Orb.from_cfg(cfg)
    return {"kind": "orbit", "cfg": cfg.to_json(), "ops": ops, "snapshots": snaps, "save": out.dump(), "text": o.text(), "loadCfg": lc.to_json(), "loadSave": out2.dump(),
            "loadSnapshot": o2.snapshot(), "equalsClone": Orb.from_cfg(cfg).equals(clone), "equalsMoved": o.equals(clone), "maxRadius": [Orb(i).max_radius() for i in range(len(BODIES))]}


# ---- VesselLocation ----------------------------------------------------------------------------------------------------------------
def gen_location(r):
    cfg = Cfg("LOCATION")
    st = {"sit": "PRELAUNCH", "fac": "VAB", "brakes": True, "site": "LaunchPad"}
    for k in r.sample(["brakesOn", "facility", "launchSite", "sit"], r.randrange(0, 5)):
        if k == "brakesOn":
            v = rng_pick(r, ["True", "False"])
            st["brakes"] = v == "True"
        elif k == "facility":
            v = rng_pick(r, ["VAB", "SPH", "None", "Bogus"])
            st["fac"] = v if v != "Bogus" else "None"  # TryGetEnum default
        elif k == "launchSite":
            v = rng_pick(r, ["LaunchPad", "Runway", "Island", ""])
            st["site"] = v
        else:
            v = rng_pick(r, ["LANDED", "PRELAUNCH", "ORBITING", "FLYING"])
            st["sit"] = v if v != "FLYING" else "PRELAUNCH"  # not defined in VesselStartSituations -> default PRELAUNCH
        cfg.values.append((k, v))
    ground = Cfg("GROUNDPOINT")
    for k in r.sample(["lon", "lat", "alt", "rot", "splashed"], r.randrange(0, 6)):
        ground.values.append((k, rng_pick(r, POOL_ROT) if k == "rot" else (rng_pick(r, ["True", "False"]) if k == "splashed" else rng_pick(r, POOL_NUM))))
    has_g = r.random() < 0.8
    if has_g:
        cfg.nodes.append(ground)
    orbit = Cfg("ORBIT")
    for k in r.sample(["SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH"], r.randrange(2, 8)):
        orbit.values.append((k, rng_pick(r, ORB_POOL[k])))
    orbit.values.append(("REF", str(r.randrange(0, 3))))
    has_o = r.random() < 0.8
    if has_o:
        cfg.nodes.append(orbit)
    # Load: orbit = zero orbit of the home body unless an ORBIT node; ground = new VesselGroundLocation (body home) loaded from GROUNDPOINT
    vg = VGL()
    vg.body = HOME
    if has_g:
        vg.load(ground)
    o = Orb.from_cfg(orbit) if has_o else Orb(HOME)
    out = Cfg("N")
    out.values += [("sit", st["sit"]), ("launchSite", st["site"]), ("facility", st["fac"]), ("brakesOn", b(st["brakes"]))]
    gn = Cfg("GROUNDPOINT")
    vg.save(gn)
    out.nodes.append(gn)
    on = Cfg("ORBIT")
    o.save(on)
    out.nodes.append(on)
    return {"kind": "location", "cfg": cfg.to_json(), "save": out.dump()}


# ---- Crew ----------------------------------------------------------------------------------------------------------------------------
def gen_crew(r):
    cfg = Cfg("PARTCREW")
    names = []
    pid = vid = 0
    for _ in range(r.randrange(0, 7)):
        k = r.choice(["crew", "crew", "crew", "vesselPersistentId", "partPersistentId"])
        if k == "crew":
            v = rng_pick(r, ["Jebediah Kerman", "Valentina Kerman", "Bob Kerman", "Bill Kerman", "Gus Kerman"])
            names.append(v)
        else:
            v = str(r.randrange(1, 4000000))
            if k == "vesselPersistentId":
                vid = int(v)
            else:
                pid = int(v)
        cfg.values.append((k, v))
    out = Cfg("N")
    out.values += [("partPersistentId", str(pid)), ("vesselPersistentId", str(vid))] + [("crew", n) for n in names]
    loc = []
    m = {"Valentina Kerman": "#autoLOC_1100005", "Bob Kerman": "#autoLOC_1100004", "Bill Kerman": "#autoLOC_1100003", "Jebediah Kerman": "#autoLOC_1100002"}
    for n in names:
        loc.append(fmt(m[n]) if n in m else n)
    return {"kind": "crew", "cfg": cfg.to_json(), "save": out.dump(), "localized": loc}


# ---- MissionCraft (hooked file system) ---------------------------------------------------------------------------------------------
def gen_craft(r):
    parts = []
    for i in range(r.randrange(1, 5)):
        p = Cfg("PART")
        p.values.append(("part", rng_pick(r, ["mk1pod_4294", "liquidEngine_4231", "fuelTank_4202"])))
        if r.random() < 0.6:
            p.values.append(("persistentId", str(1000 + i)))
        parts.append(p)
    top = Cfg("N")
    top.values.append(("ship", rng_pick(r, ["Alpha", "Beta Probe"])))
    has_id = r.random() < 0.5
    if has_id:
        top.values.append(("persistentId", "777"))
    top.nodes += parts
    folder = rng_pick(r, ["ships/VAB/", "ships/SPH", "ships/"])
    where = rng_pick(r, ["", "VAB/", "SPH/"]) if folder.rstrip("/").split("/")[-1] not in ("VAB", "SPH") else ""
    return {"kind": "craft", "craft": top.to_json(), "folder": folder, "where": where, "file": "test.craft", "hasId": has_id}
