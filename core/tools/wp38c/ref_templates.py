"""WP-38 slice 3 reference, part 4: the pre-built craft model and the three WP-37 deferred templates RoverConstructionContract, OrbitalConstructionContract and
VesselRepairContract (Contracts/Templates/*.cs), ContractDefs.GetPreBuiltCraft / GetPreBuiltCraftPositions / LoadConstructionPartsList,
SystemUtilities.ValidateLoadCraftNode and CraftProfileInfo.LoadDetailsFromCraftFile (the validation half).

Independent of kspcore. Real game data: the Squad Contracts cfgs (PREBUILTPOSITIONS, CONSTRUCTION_PART), the stock part cfgs (names, titles, ModuleCargoPart), the
decompiled C#. NOT available in the repo: the stock .craft files of the pre-built craft (the mirror keeps them as empty files), so the craft files and the
PREBUILTCRAFT definitions that point to them are SYNTHETIC fixtures made here (small valid and invalid craft built from stock part names), written into a
temp folder by the C++ test; the positions and the construction parts list are the real ones.

Fixtures: `new KSPRandom()` = KSPRandom(4242) (ContractsEnvironment::newRandomSeed); SiteGeneratorInfo = SITE_FIX of tools/wp38b (AlphaNumericDesignation);
PartModelPurchased = true; ResearchAndDevelopment absent (every tech node Available); no contracts in the system; UnityEngine.Random = the contract seed.
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
for sub in ("wp37", "wp38", "wp38b"):
    sys.path.insert(0, os.path.join(CORE, "tools", sub))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402
import fineprint2_reference as w38b  # noqa: E402
import ref_contractdefs as rcd  # noqa: E402
import ref_sentinel as rs  # noqa: E402

KEY = "templates"
F = w37.F
LOC = w38b.LOC
BD = w38.BD
HOME = w38.HOME
KSPRandom = w38.KSPRandom
SEED = w38b.RANDOM_SEED_FIXTURE
SQUAD_CONTRACTS = os.path.join(w37.SQUAD, "Contracts")


# ---------------------------------------------------------------------------------------------------------------- game data
def part_cfgs():
    """name (with '.') -> {title tag, modules [(name, node)]}"""
    out = {}
    for dp, _dn, fn in os.walk(os.path.join(w37.SQUAD, "Parts")):
        for f in fn:
            if f.endswith(".cfg"):
                try:
                    n = w37.parse_cfg(os.path.join(dp, f))
                except Exception:
                    continue
                for pn in n["nodes"]:
                    if pn["name"] == "PART":
                        nm = w37.val(pn, "name")
                        if nm:
                            mods = [(w37.val(m, "name"), m) for m in pn["nodes"] if m["name"] == "MODULE"]
                            out[nm.replace("_", ".")] = {"title": w37.val(pn, "title"), "modules": mods}
    return out


PARTS = part_cfgs()


def part_title(name):
    t = PARTS[name]["title"]
    return LOC.fmt(t) if t and t.startswith("#") else t


def module_classes():
    """every class name below PartModule in Assembly-CSharp (Type.Name)"""
    root = os.environ.get("KSP_CS_ROOT", r"D:/Projects/KSP-UE5/decompiled/KSP-Decompiled/Assembly-CSharp")
    base = {}
    rx = re.compile(r"^\s*(?:public |internal |abstract |sealed |static |partial )*class (\w+)(?:<[^>]*>)?\s*:\s*([\w.]+)")
    for dp, _dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(".cs"):
                for line in open(os.path.join(dp, f), encoding="utf-8-sig", errors="replace"):
                    m = rx.match(line)
                    if m:
                        base[m.group(1)] = m.group(2).split(".")[-1]
    out = {"PartModule"}
    changed = True
    while changed:
        changed = False
        for c, b in base.items():
            if b in out and c not in out:
                out.add(c)
                changed = True
    return out


MODULE_CLASSES = module_classes()


def cargo_volume(part):
    for nm, node in PARTS[part]["modules"]:
        if nm == "ModuleCargoPart":
            v = w37.val(node, "packedVolume")
            return F(float(v)) if v is not None else 0.0
    return None


# ---------------------------------------------------------------------------------------------------------------- synthetic craft + definitions
def craft_text(ship, parts, version="1.12.5"):
    """parts: [(partName, [(moduleName, {k: v})])]"""
    lines = []
    if ship is not None:
        lines.append("ship = %s" % ship)
    if version is not None:
        lines.append("version = %s" % version)
    lines.append("type = VAB")
    for i, (pn, mods) in enumerate(parts):
        lines += ["PART", "{", "\tpart = %s_%d" % (pn, 4294750000 + i)]
        for mn, kv in mods:
            lines += ["\tMODULE", "\t{", "\t\tname = %s" % mn]
            for k, v in kv.items():
                lines.append("\t\t%s = %s" % (k, v))
            lines += ["\t}"]
        lines += ["}"]
    return "\n".join(lines) + "\n"


BROKEN = ("ModuleDeployableSolarPanel", {"deployState": "BROKEN"})
OKPANEL = ("ModuleDeployableSolarPanel", {"deployState": "RETRACTED"})
CMD = ("ModuleCommand", {})


def synthetic():
    """(cfg text, {relative path: craft text})"""
    base = "GameData/Squad/Contracts/PreBuiltCraft/Synthetic/"
    crafts = {}
    entries = []

    def add(ctype, name, text, ground=True, orbit=False, water=False, pos=True, broken=False, types=True, missing=False, extra=""):
        url = base + name + ".craft"
        if not missing:
            crafts[url] = text
        e = ["\tPREBUILTCRAFT", "\t{", "\t\turl = " + url, "\t\tAllowGround = %s" % ground, "\t\tAllowOrbit = %s" % orbit, "\t\tAllowWater = %s" % water,
             "\t\tUsePreBuiltPos = %s" % pos]
        if broken:
            e.append("\t\tbrokenParts = true")
        if types:
            e.append("\t\tContractType = %s" % ctype)
        if extra:
            e.append(extra)
        e += ["\t}"]
        entries.append("\n".join(e))

    ok = lambda n: craft_text("Rover %s" % n, [("mk1pod.v2", [CMD]), ("solarPanels1", [OKPANEL])])
    add("RoverConstruction", "R0", ok("R0"), ground=True, orbit=False, water=False, pos=True)
    add("RoverConstruction", "R1", ok("R1"), pos=False)
    add("RoverConstruction", "R2", craft_text("Rover R2", [("mk1pod.v2", [CMD, ("ModuleInventoryPart", {"inventory": "solarPanels1,solarPanels2"})])]), water=True, pos=True)
    add("RoverConstruction", "R3", craft_text("Rover R3", [("noSuchPart", [])]))
    add("RoverConstruction", "R4", craft_text("Rover R4", [("mk1pod.v2", [("ModuleNoSuchThing", {})])]))
    add("RoverConstruction", "R5", "", missing=True)
    add("RoverConstruction", "R6", ok("R6"), types=False)
    add("RoverConstruction", "R7", craft_text("Rover R7", [("mk1pod.v2", [CMD])], version="0.10.0"))
    add("RoverConstruction", "R8", craft_text("Rover R8", [("mk1pod.v2", [("ModuleInventoryPart", {"inventory": "solarPanels1,noSuchInvPart"})])]))
    add("RoverConstruction", "R9", craft_text("Rover R9", [("mk1pod.v2", [CMD])], version=None))
    # vessel repair: exactly one broken part per usable craft
    one = lambda n, part="solarPanels1": craft_text("Repair %s" % n, [("mk1pod.v2", [CMD]), (part, [BROKEN]), ("solarPanels2", [OKPANEL])])
    add("VesselRepair", "P0", one("P0"), ground=False, orbit=True, pos=False, broken=True)
    add("VesselRepair", "P1", craft_text("Repair P1", [("solarPanels1", [BROKEN]), ("solarPanels2", [BROKEN])]), ground=False, orbit=True, pos=False, broken=True)
    add("VesselRepair", "P2", craft_text("Repair P2", [("solarPanels1", [OKPANEL])]), ground=False, orbit=True, pos=False, broken=True)
    add("VesselRepair", "P3", one("P3", "solarPanels2"), ground=True, orbit=False, pos=True, broken=True)
    add("VesselRepair", "P4", one("P4"), ground=True, orbit=True, pos=False, broken=True)
    add("VesselRepair", "P5", craft_text("Repair P5", [("noSuchPart", [BROKEN])]), ground=False, orbit=True, broken=True)
    add("VesselRepair", "P6", one("P6"), ground=True, orbit=False, pos=True, water=True, broken=True)
    add("VesselRepair", "P7", one("P7"), ground=True, orbit=False, pos=False, broken=True)
    add("VesselRepair", "P8", one("P8"), ground=False, orbit=False, pos=False, broken=True)
    add("VesselRepair", "P9", one("P9", "mk1pod.v2"), ground=False, orbit=True, pos=False, broken=False)
    for i in range(4):
        add("OrbitalConstruction", "O%d" % i, craft_text("Unfinished O%d" % i, [("mk1pod.v2", [CMD]), ("solarPanels1", [OKPANEL])]), ground=False, orbit=True, pos=False)
    add("OrbitalConstruction", "O4", craft_text("Unfinished O4", [("noSuchPart", [])]), ground=False, orbit=True, pos=False)
    return "CONTRACTCRAFT\n{\n" + "\n".join(entries) + "\n}\n", crafts


def parse_craft(text):
    """ConfigNode.Load of a craft file"""
    lines = []
    root = {"values": [], "nodes": []}
    stack = [root]
    pending = None
    for raw in text.replace("\r", "").split("\n"):
        l = raw.split("//")[0].strip()
        if not l:
            continue
        if l == "{":
            n = {"name": pending, "values": [], "nodes": []}
            stack[-1]["nodes"].append(n)
            stack.append(n)
        elif l == "}":
            stack.pop()
        elif "=" in l:
            k, v = l.split("=", 1)
            stack[-1]["values"].append((k.strip(), v.strip()))
        else:
            pending = l
    return root


def values(node, key):
    return [v for k, v in node["values"] if k == key]


def first(node, key):
    r = values(node, key)
    return r[0] if r else None


def parse_bool(s):
    return s.lower() == "true"


def check_version(ver):
    """KSPUtil.CheckVersion(versionString, 0, 18, 0): COMPATIBLE / TOO_EARLY / TOO_LATE / INVALID"""
    parts = ver.split(".")
    try:
        a, b, c = [int(x) for x in parts[:3]]
    except ValueError:
        return "INVALID"
    return rcd_check(a, b, c, 0, 18, 0)


def rcd_check(vmaj, vmin, vrev, lmaj, lmin, lrev):
    flag = False
    if vmaj > lmaj:
        flag = True
    elif vmaj == lmaj:
        if vmin > lmin:
            flag = True
        elif vmin == lmin and vrev >= lrev:
            flag = True
    if vmaj <= 1 and (vmaj != 1 or vmin <= 12) and (vmaj != 1 or vmin != 12 or vrev <= 5):
        return "COMPATIBLE" if flag else "TOO_EARLY"
    return "TOO_LATE"


def validate_craft(text, broken_parts):
    """ValidateLoadCraftNode(bypassTechCheck: true): the definition's craft facts, or None"""
    if text is None:
        return None
    node = parse_craft(text)
    if not node["values"] and not node["nodes"]:
        return None
    compat = "INVALID"
    ver = first(node, "version")
    if ver is not None:
        compat = check_version(ver)
    unavailable_parts, unavailable_mods, broken = [], [], []
    for pn in [n for n in node["nodes"] if n["name"] == "PART"]:
        v = first(pn, "part")
        if v is None:
            continue
        if "_" not in v:
            raise KeyError("ArgumentOutOfRange")
        name = v[:v.index("_")]
        if name not in PARTS and name not in unavailable_parts:
            unavailable_parts.append(name)
        for mn in [n for n in pn["nodes"] if n["name"] == "MODULE"]:
            mname = first(mn, "name")
            if mname is None:
                continue
            if mname not in MODULE_CLASSES and mname not in unavailable_mods:
                unavailable_mods.append(mname)
            if mname == "ModuleInventoryPart":
                inv = first(mn, "inventory")
                if inv is not None:
                    for ip in inv.split(","):
                        if ip not in PARTS and ip not in unavailable_parts:
                            unavailable_parts.append(ip)
            if broken_parts:
                if first(mn, "deployState") == "BROKEN" and name not in broken:
                    broken.append(name)
    if compat != "COMPATIBLE" or unavailable_parts or unavailable_mods:
        return None
    if broken_parts and len(broken) < 1:
        return None
    return {"broken": broken}


def load_definitions(cfg_text, crafts):
    """ContractDefs.GetPreBuiltCraft: [definition dict] in cfg order (only those with ContractType)"""
    cfg = parse_craft(cfg_text)["nodes"][0]
    out = []
    for n in cfg["nodes"]:
        if n["name"] != "PREBUILTCRAFT":
            continue
        url = first(n, "url")
        if url is None:
            continue
        bp = first(n, "brokenParts")
        broken_parts = parse_bool(bp) if bp is not None else False
        d = validate_craft(crafts.get(url), broken_parts)
        if d is None:
            continue
        d = dict(d)
        d["url"] = url
        for key, fld, default in (("AllowGround", "allowGround", True), ("AllowOrbit", "allowOrbit", False), ("AllowWater", "allowWater", False),
                                  ("UsePreBuiltPos", "usePos", True)):
            v = first(n, key)
            d[fld] = parse_bool(v) if v is not None else default
        types = values(n, "ContractType")
        if types:
            d["types"] = types
            out.append(d)
    return out


def load_positions():
    cfg = rcd.get_node(rcd.parse_cfg(os.path.join(SQUAD_CONTRACTS, "PrebuiltCraft", "prebuiltData.cfg")), "CONTRACTCRAFT")
    out = []  # [(body, [(lat, lon, radius)])] insertion ordered
    for n in cfg.nodes:
        if n.name != "PREBUILTPOSITIONS":
            continue
        v = {k: x for k, x, _ in n.values}
        body = v.get("Body")
        if body is None or body not in BD:
            continue
        lat = rcd.f32_from_decimal(v["CenterLatitude"]) if "CenterLatitude" in v else np.float32(3.4028234663852886e38)
        lon = rcd.f32_from_decimal(v["CenterLongitude"]) if "CenterLongitude" in v else np.float32(3.4028234663852886e38)
        rad = rcd.f32_from_decimal(v["SearchRadius"]) if "SearchRadius" in v else np.float32(3.4028234663852886e38)
        for b, lst in out:
            if b == body:
                lst.append((float(lat), float(lon), float(rad)))
                break
        else:
            out.append((body, [(float(lat), float(lon), float(rad))]))
    return out


def load_construction_parts():
    cfg = rcd.get_node(rcd.parse_cfg(os.path.join(SQUAD_CONTRACTS, "constructionContractParts.cfg")), "CONTRACT_CONSTRUCTION_PARTS")
    lst = []
    for n in cfg.nodes:
        if n.name != "CONSTRUCTION_PART":
            continue
        v = {k: x for k, x, _ in n.values}
        types = [x for k, x, _ in n.values if k == "contract"]
        name = v.get("name", "")
        if not name or not types:
            continue
        name = name.replace("_", ".")
        vol = cargo_volume(name) if name in PARTS else None
        ok = vol is not None and vol >= 0.0
        if ok:
            if not any(p["name"] == name for p in lst):
                lst.append({"name": name, "types": types})
    return lst


# ---------------------------------------------------------------------------------------------------------------- cache semantics (lists are handed out by reference)
class Cache:
    def __init__(self, defs, parts):
        self.defs, self.parts = defs, parts
        self.craft_lists, self.part_lists = {}, {}

    def craft(self, ctype):
        if ctype not in self.craft_lists:
            self.craft_lists[ctype] = [d for d in self.defs if ctype in d["types"]]
        return self.craft_lists[ctype]

    def construction(self, ctype):
        if ctype not in self.part_lists:
            self.part_lists[ctype] = [p for p in self.parts if ctype in p["types"]]
        return self.part_lists[ctype]


# ---------------------------------------------------------------------------------------------------------------- generation
PRESTIGE = ["Trivial", "Significant", "Exceptional"]
SEC = None


def finish2(ctype, story_key, p, seed, mult, guid, rng, kws, c, params, title, synopsis, topic, subject, extra):
    agent = w37.suitable_agent(rng, kws, [])
    if agent is None:
        return None
    contract = {"fa": c.fa, "fc": c.fc, "ff": c.ff, "rc": c.rc, "rf": c.rf, "sc": c.sc, "expiry": c.expiry, "deadline": c.deadline}
    for nm, f in agent["mentality"]:
        if w37.can_process(nm, kws, []):
            w37.apply_mentality(contract, nm, f, p)
    contract["ff"] = max(contract["ff"], contract["fa"])
    ss = str(w37.super_seed(guid, seed))
    num = w37.wrap64(391 + w37.hash35(ctype))
    num = w37.wrap64(num * 23 + w37.hash35(ss))
    for pr in params:
        num = w37.wrap64(num * 23 + w37.hash35(pr["type"]))
        if pr["hash"]:
            num = w37.wrap64(num * 23 + w37.hash35(pr["hash"]))
    desc = w37.back_story(story_key, agent["title"], topic, subject, seed, False, False, False)
    res = {"type": ctype, "prestige": PRESTIGE[p], "seed": seed, "agent": agent["name"], "agentTitle": agent["title"], "keywords": kws, "title": title,
           "synopsis": synopsis, "description": desc, "contractID": num, "params": params, "fa": contract["fa"], "fc": contract["fc"], "ff": contract["ff"],
           "rc": contract["rc"], "rf": contract["rf"], "sc": contract["sc"], "expiry": contract["expiry"], "deadline": contract["deadline"]}
    res.update(extra)
    return res


def weighted_body_names(rng, names, W):
    if not names:
        return None
    return w37.weighted_body(rng, [w37.BODY[n] for n in names], W)["name"]


def vessel_name(tags, body, seed):
    k = KSPRandom(seed)
    return LOC.fmt(tags[k.next_range(0, len(tags))], w38b.BD_DISPLAY(body), w38b.alpha_numeric(seed))


def engineer_param(ss):
    traits = {}
    for nd in w37.parse_cfg(os.path.join(w37.SQUAD, "Experience", "Traits.cfg"))["nodes"]:
        if nd["name"] == "EXPERIENCE_TRAIT":
            traits[w37.val(nd, "name")] = w37.val(nd, "title")
    desc = LOC.fmt("#autoLOC_8004217")
    disp = traits.get("Engineer", "Engineer").lower()
    return {"type": "CrewTraitParameter", "hash": ss, "title": LOC.fmt("#autoLOC_6001095", disp, desc)}


def choose_position_gen(bname, water_allowed, equatorial, g):
    """WaypointManager.ChooseRandomPosition with the caller's generator"""
    b = BD[bname]
    lat = lon = 0.0
    if not b["solid"]:
        water_allowed = True
    if b["ocean"] and not water_allowed:
        num = 10000
        while num >= 0:
            if not equatorial:
                n2 = 1.0 - g.next_double() * 2.0
                lat = math.asin(n2) * 57.295780181884766
            else:
                lat = 0.0
            lon = g.next_double()
            num -= 1
            if num >= 0 and not (0.0 > 0.0):
                continue
            break
    else:
        if not equatorial:
            n3 = 1.0 - g.next_double() * 2.0
            lat = math.asin(n3) * 57.295780181884766
        else:
            lat = 0.0
        lon = g.next_double()
    return lat, lon


def choose_position_gen_exact(bname, water_allowed, equatorial, g):
    """as w38.choose_random_position but with the caller's generator (the 10001-iteration underwater loop of an ocean body is kept)"""
    b = BD[bname]
    lat = lon = 0.0
    if not b["solid"]:
        water_allowed = True
    if b["ocean"] and not water_allowed:
        num = 10000
        while num >= 0:
            if not equatorial:
                n2 = g.next_double()
                n2 = 1.0 - n2 * 2.0
                lat = math.asin(n2) * 57.295780181884766
            else:
                lat = 0.0
            lon = g.next_double()
            if not (0.0 > 0.0):
                num -= 1
                continue
            break
    else:
        if not equatorial:
            n3 = g.next_double()
            n3 = 1.0 - n3 * 2.0
            lat = math.asin(n3) * 57.295780181884766
        else:
            lat = 0.0
        lon = g.next_double()
    return lat, lon


def reward_block(c, V, sec, p, num4, body):
    pre = PRESTIGE[p]
    mm = lambda a, b: F(a * num4 * b)  # Mono rule: one rounding where RoundToInt takes the float
    r = lambda x: float(w37.round_to_int(F(x)))
    c.set_science(r(mm(V("%s.Science.BaseReward" % sec), V("%s.%s.ScienceMultiplier" % (sec, pre)))), body)
    c.set_rep(r(mm(V("%s.Reputation.BaseReward" % sec), V("%s.%s.ReputationMultiplier" % (sec, pre)))),
              r(mm(V("%s.Reputation.BaseFailure" % sec), V("%s.%s.ReputationMultiplier" % (sec, pre)))), body)
    c.set_funds(r(mm(V("%s.Funds.BaseAdvance" % sec), V("%s.%s.FundsMultiplier" % (sec, pre)))), r(mm(V("%s.Funds.BaseReward" % sec), V("%s.%s.FundsMultiplier" % (sec, pre)))),
                r(mm(V("%s.Funds.BaseFailure" % sec), V("%s.%s.FundsMultiplier" % (sec, pre)))), body)


def has_parent(name, ancestor):
    b = BD[name]
    while True:
        if b["parent"] == ancestor:
            return True
        if b["parent"] is None:
            return False
        b = BD[b["parent"]]


def gen_rover(V, cache, positions, seed, p, s, W, mult, guid, rseed):
    where = lambda n: n != HOME
    first_list = w38b.bp(s, "ORBIT", True, "ANY", where, reached=True)
    if not first_list:  # MeetRequirements (the technology state is Available without R&D)
        return None
    rng = w37.Unity(seed)
    c = w38b.CC(p, mult)
    defs = cache.craft("RoverConstruction")
    if not defs:
        return None
    mn, mx = F(V("RoverConstruction.MinimumGeeASL")), F(V("RoverConstruction.MaximumGeeASL"))
    bodies = [n for n in first_list if BD[n]["solid"] and not (BD[n]["gee"] < float(mn) or BD[n]["gee"] > float(mx))]
    target = weighted_body_names(rng, bodies, W)
    k = KSPRandom(rseed)
    d = defs[k.next_range(0, len(defs) - 1)]
    if target is None:
        return {"exception": "NullReferenceException"}
    flag = False
    plist = next((lst for b, lst in positions if b == target), None)
    if d["usePos"] and plist is not None:
        c1, c2, c3 = plist[k.next_range(0, len(plist) - 1)]
        slat, slon = w38.choose_random_position_near(target, c1, c2, c3, d["allowWater"], rseed)
        flag = True
    if not flag:
        slat, slon = choose_position_gen_exact(target, d["allowWater"], False, k)
    wmin, wmax = F(V("RoverConstruction.WayPointMinDistance")), F(V("RoverConstruction.WayPointMaxDistance"))
    pre = PRESTIGE[p]
    mulf = F(V("RoverConstruction.%s.WayPointMultiplier" % pre))
    wmin, wmax = F(wmin * mulf), F(wmax * mulf)
    g2 = KSPRandom(rseed)
    elat, elon, _ = w38.randomize_near(target, slat, slon, float(wmin), float(wmax), d["allowWater"], g2)
    num4 = 1.0
    ss = w38b.ss_hash(guid, seed)
    params = [{"type": "RoverWayPointParameter", "hash": ss, "title": LOC.fmt("#autoLOC_6002560"), "start": [slat, slon], "end": [elat, elon]}]
    kws = []
    w37.add_keywords(kws, "Commercial")
    c.set_expiry_draw(rng.range_i(V("RoverConstruction.Expire.MinimumExpireDays"), V("RoverConstruction.Expire.MaximumExpireDays")))
    c.set_deadline_days(float(F(F(float(V("RoverConstruction.Expire.DeadlineDays"))) * num4)), target)
    reward_block(c, V, "RoverConstruction", p, num4, target)
    return finish2("RoverConstructionContract", "RoverConstruction", p, seed, mult, guid, rng, kws, c, params, LOC.fmt("#autoLOC_6002550", w38b.BD_DISPLAY(target)),
                   LOC.fmt("#autoLOC_6002551"), w38b.BD_DISPLAY(target), "",
                   {"targetBody": target, "craftURL": d["url"], "start": [slat, slon], "end": [elat, elon], "completed": LOC.fmt("#autoLOC_6002552")})


def orbit_for(seed, body, alt, inc, ecc):
    return w38.gen_orbit_raw(seed, body, "RANDOM", alt, inc, ecc)


def choose_target(rng, s, W, p, ptype):
    """OrbitalConstruction / VesselRepair target choice: returns (status, body) status in ok / refuse"""
    lst = w38b.bp(s, ptype, True, "MANNED", lambda n: True, reached=True)
    return lst


def gen_orbital(V, cache, positions, seed, p, s, W, mult, guid, rseed):
    # MeetRequirements: the technology state is Available without R&D
    rng = w37.Unity(seed)
    c = w38b.CC(p, mult)
    defs = cache.craft("OrbitalConstruction")
    if not defs:
        return None
    lst = w38b.bp(s, "FLYBY", True, "MANNED", lambda n: True, reached=True)
    target = None
    if p == 0:
        if s.sc.complete(HOME, "ORBIT", "MANNED"):
            target = HOME
        else:
            return None
    elif p == 1:
        lst = [n for n in lst if has_parent(n, HOME)]
        if not lst:
            return None
        target = weighted_body_names(rng, lst, W)
    else:
        lst = [n for n in lst if not (BD[n]["home"] or n == "Sun" or has_parent(n, HOME))]
        if not lst:
            return None
        target = weighted_body_names(rng, lst, W)
    if target is None:
        return None
    k = KSPRandom(rseed)
    d = defs[k.next_range(0, len(defs) - 1)]
    plist = cache.construction("OrbitalConstruction")
    if not plist:
        return None
    part = plist[k.next_range(0, len(plist) - 1)]["name"]
    pre = PRESTIGE[p]
    ecc = max(0.0, min(float(F(V("OrbitalConstruction.%s.MaxOrbitEccentricityFactor" % pre))), k.next_double() / 2.0))
    alt = float(F(V("OrbitalConstruction.%s.OrbitAltitudeFactor" % pre)))
    inc = float(F(V("OrbitalConstruction.%s.OrbitInclinationFactor" % pre)))
    el = orbit_for(seed, target, alt, inc, ecc)
    vname = vessel_name(["#autoLOC_%d" % n for n in range(6002605, 6002617)], target, seed)
    num4 = 1.0
    ss = w38b.ss_hash(guid, seed)
    params = [engineer_param(ss),
              {"type": "ConstructionParameter", "hash": part + "0", "title": LOC.fmt("#autoLOC_6002602", part_title(part), vname)}]
    kws = []
    w37.add_keywords(kws, "Commercial")
    # sic: the C# reads the RoverConstruction section
    c.set_expiry_draw(rng.range_i(V("RoverConstruction.Expire.MinimumExpireDays"), V("RoverConstruction.Expire.MaximumExpireDays")))
    c.set_deadline_days(float(F(F(float(V("RoverConstruction.Expire.DeadlineDays"))) * num4)), target)
    reward_block(c, V, "RoverConstruction", p, num4, target)
    return finish2("OrbitalConstructionContract", "OrbitConstruction", p, seed, mult, guid, rng, kws, c, params, LOC.fmt("#autoLOC_6002592", w38b.BD_DISPLAY(target)),
                   LOC.fmt("#autoLOC_6002599", part_title(part)), w38b.BD_DISPLAY(target), "",
                   {"targetBody": target, "craftURL": d["url"], "partName": part, "vesselName": vname, "eccentricity": ecc, "altitudeFactor": alt, "inclinationFactor": inc,
                    "orbit": el, "completed": LOC.fmt("#autoLOC_6002603")})


def gen_repair(V, cache, positions, seed, p, s, W, mult, guid, rseed):
    # MeetRequirements: evaRepairKit exists and its model is purchased (PartModelPurchased true)
    if "evaRepairKit" not in PARTS:
        return None
    rng = w37.Unity(seed)
    c = w38b.CC(p, mult)
    defs = cache.craft("VesselRepair")
    defs[:] = [d for d in defs if len(d["broken"]) == 1]
    if not defs:
        return None
    lst = w38b.bp(s, "ORBIT", True, "MANNED", lambda n: True, reached=True)
    if not lst:
        return None
    if p == 0:
        if s.sc.complete(HOME, "ORBIT", "MANNED"):
            target = HOME
        else:
            return None
    elif p == 1:
        lst = [n for n in lst if has_parent(n, HOME)]
        if not lst:
            return None
        target = weighted_body_names(rng, lst, W)
    else:
        lst = [n for n in lst if not (BD[n]["home"] or n == "Sun" or has_parent(n, HOME))]
        if not lst:
            return None
        target = weighted_body_names(rng, lst, W)
    if target is None:
        return None
    k = KSPRandom(rseed)
    d = defs[k.next_range(0, len(defs) - 1)]
    part = d["broken"][k.next_range(0, len(d["broken"]) - 1)]
    if part not in PARTS:
        return None
    on_ground = in_orbit = False
    if d["allowGround"] and d["allowOrbit"]:
        if k.next_range(0, 1) == 1:
            on_ground = True
        else:
            in_orbit = True
    elif d["allowGround"]:
        on_ground = True
    elif d["allowOrbit"]:
        in_orbit = True
    if not in_orbit and not on_ground:
        return None
    slat = slon = 0.0
    ecc, alt, inc, el = 0.0, 0.5, 0.5, None
    pre = PRESTIGE[p]
    if on_ground:
        flag = False
        plist = next((lst2 for b, lst2 in positions if b == target), None)
        if d["usePos"] and plist is not None:
            c1, c2, c3 = plist[k.next_range(0, len(plist) - 1)]
            slat, slon = w38.choose_random_position_near(target, c1, c2, c3, d["allowWater"], rseed)
            flag = True
        if not flag:
            slat, slon = choose_position_gen_exact(target, d["allowWater"], False, k)
    elif in_orbit:
        ecc = max(0.0, min(float(F(V("VesselRepair.%s.MaxOrbitEccentricityFactor" % pre))), k.next_double() / 2.0))
        alt = float(F(V("VesselRepair.%s.OrbitAltitudeFactor" % pre)))
        inc = float(F(V("VesselRepair.%s.OrbitInclinationFactor" % pre)))
        el = orbit_for(seed, target, alt, inc, ecc)
    vname = vessel_name(["#autoLOC_%d" % n for n in range(6002581, 6002592)], target, seed)
    num4 = 1.0
    ss = w38b.ss_hash(guid, seed)
    params = [engineer_param(ss),
              {"type": "RepairPartParameter", "hash": part + "0", "title": LOC.fmt("#autoLOC_6002578", vname, part_title(part), w38b.BD_DISPLAY(target))}]
    kws = []
    w37.add_keywords(kws, "Commercial")
    c.set_expiry_draw(rng.range_i(V("VesselRepair.Expire.MinimumExpireDays"), V("VesselRepair.Expire.MaximumExpireDays")))
    c.set_deadline_days(float(F(F(float(V("VesselRepair.Expire.DeadlineDays"))) * num4)), target)
    reward_block(c, V, "VesselRepair", p, num4, target)
    return finish2("VesselRepairContract", "VesselRepair", p, seed, mult, guid, rng, kws, c, params, LOC.fmt("#autoLOC_6002568", w38b.BD_DISPLAY(target)),
                   LOC.fmt("#autoLOC_6002576", part_title(part)), w38b.BD_DISPLAY(target), "",
                   {"targetBody": target, "craftURL": d["url"], "partName": part, "vesselName": vname, "spawnOnGround": on_ground, "spawnInOrbit": in_orbit,
                    "start": [slat, slon], "eccentricity": ecc, "altitudeFactor": alt, "inclinationFactor": inc, "orbit": el, "completed": LOC.fmt("#autoLOC_6002579")})


SCENARIOS4 = rs.SCENARIOS3 + [
    {"name": "manned-mun-only", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Orbit": "completedManned"}, "Mun": {"Flyby": "completedManned", "Orbit": "completedManned"}}},
    {"name": "manned-far", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Orbit": "completedManned"}, "Duna": {"Flyby": "completedManned", "Orbit": "completedManned"}, "Ike": {"Flyby": "completedManned", "Orbit": "completed"},
                "Eve": {"Flyby": "completedManned", "Orbit": "completedManned"}, "Jool": {"Flyby": "completedManned"}, "Laythe": {"Flyby": "completedManned", "Orbit": "completedManned"}}},
    {"name": "unmanned-orbits", "top": {"FirstLaunch": "completed", "ReachedSpace": "completed"},
     "bodies": {"Kerbin": {"Orbit": "completedUnmanned"}, "Mun": {"Orbit": "completedUnmanned", "Flyby": "completedUnmanned"}, "Minmus": {"Orbit": "completed", "Flyby": "completed"},
                "Duna": {"Orbit": "completedUnmanned"}}},
]


def build(cs_root, mirror):
    loaded, defaults = rs.defs(cs_root, mirror)
    V = lambda path: (F(float(defaults[path][1])) if False else None)
    # ContractDefs values: the loaded cfg value of every key (RoverConstruction, VesselRepair, OrbitalConstruction sections)
    def Vf(path):
        t = defaults[path][0]
        x = loaded[path]
        return F(float(x)) if t == "float" else x

    cfg_text, crafts = synthetic()
    defs = load_definitions(cfg_text, crafts)
    positions = load_positions()
    parts = load_construction_parts()
    W = w38b.weights()
    out = {"scenarios": SCENARIOS4, "cfg": cfg_text, "crafts": crafts, "definitions": defs, "positions": positions, "constructionParts": parts, "weights": W}
    cases = []
    gens = (("RoverConstructionContract", gen_rover), ("OrbitalConstructionContract", gen_orbital), ("VesselRepairContract", gen_repair))
    for si, scn in enumerate(SCENARIOS4):
        s = w38.Scn(scn["bodies"], scn["top"])
        cache = Cache(defs_copy(defs), parts_copy(parts))
        for ctype, fn in gens:
            for p in range(3):
                for seed in range(1, 13):
                    for mi, mult in enumerate(w37.MULT_SETS[:2]):
                        if mi == 1 and seed > 4:
                            continue
                        sd = seed * 7001 + 13 + p
                        g = bytes(((b + seed * 3 + p) & 0xFF) for b in w38b.GUID)
                        rseed = 1000 + (sd * 13 + mi * 7) % 97  # the value `new KSPRandom()` takes in this case (ContractsEnvironment::newRandomSeed)
                        try:
                            r = fn(Vf, cache, positions, sd, p, s, W, mult, g, rseed)
                        except IndexError:
                            r = {"exception": "IndexError"}
                        cases.append({"type": ctype, "scenario": si, "prestige": p, "seed": sd, "mult": mi, "guid": list(g), "rseed": rseed, "result": r})
    out["contracts"] = cases
    gen = sum(1 for c in cases if c["result"] and "exception" not in c["result"])
    ex = sum(1 for c in cases if c["result"] and "exception" in c["result"])
    by = {}
    for c in cases:
        k = (c["type"], bool(c["result"]) and "exception" not in c["result"])
        by[k] = by.get(k, 0) + 1
    print("template contracts: %d cases, %d generated, %d exceptions: %s" % (len(cases), gen, ex, sorted(by.items())))
    out["moduleClassCount"] = len(MODULE_CLASSES)
    return out


def defs_copy(defs):
    return [dict(d, broken=list(d["broken"]), types=list(d["types"])) for d in defs]


def parts_copy(parts):
    return [dict(p, types=list(p["types"])) for p in parts]
