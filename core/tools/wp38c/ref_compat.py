"""WP-38 slice 3 reference, part 3: FinePrint.Utilities.CompatibilityUtilities (save upgrade of old contract nodes).

Independent of kspcore: a Python model of the ConfigNode operations the C# uses (AddValue, SetValue, RemoveValues, GetNodes, nodes.Remove ...), of
OldCareerSave / SavePriorTo / UpgradeVesselSituation / UpdateCareerSave / CleanUpUnsanitaryEVAKerbals, read from FinePrint/Utilities/CompatibilityUtilities.cs.

Fixtures that are NOT KSP data: no old-format contract save exists in the repo (the shipped saves are 1.12.5), so the input nodes are synthetic: they are built
here from the formats the C# upgrades (every branch of UpdateCareerSave is hit; each case varies one thing). The expected output is the model's.
"""
import itertools
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
for sub in ("wp37", "wp38", "wp38b"):
    sys.path.insert(0, os.path.join(CORE, "tools", sub))
import contract_reference as w37  # noqa: E402
import fineprint_reference as w38  # noqa: E402
import fineprint2_reference as w38b  # noqa: E402
import ref_contractdefs as rcd  # noqa: E402

KEY = "compat"
LOC = w38b.LOC
VER = (1, 12, 5)
INT_MAX = 2147483647


class N:
    def __init__(self, name, values=None, nodes=None):
        self.name = name
        self.values = [list(v) for v in (values or [])]
        self.nodes = list(nodes or [])

    def has_value(self, k):
        return any(v[0] == k for v in self.values)

    def get_value(self, k):
        for v in self.values:
            if v[0] == k:
                return v[1]
        return None

    def get_values(self, k):
        return [v[1] for v in self.values if v[0] == k]

    def add_value(self, k, v):
        if isinstance(v, bool):
            v = "True" if v else "False"
        self.values.append([k, v])

    def set_value(self, k, v, create=False):
        for e in self.values:
            if e[0] == k:
                e[1] = v
                return True
        if create:
            self.values.append([k, v])
        return False

    def remove_values(self, k):
        self.values = [v for v in self.values if v[0] != k]

    def get_nodes(self, name):
        return [n for n in self.nodes if n.name == name]

    def get_node(self, name):
        for n in self.nodes:
            if n.name == name:
                return n
        return None

    def to_json(self):
        return {"name": self.name, "values": self.values, "nodes": [n.to_json() for n in self.nodes]}

    @staticmethod
    def from_json(j):
        return N(j["name"], j["values"], [N.from_json(x) for x in j["nodes"]])

    def render(self, depth=0, out=None):
        out = [] if out is None else out
        ind = "\t" * depth
        out.append(ind + self.name)
        out.append(ind + "{")
        for k, v in self.values:
            out.append(ind + "\t" + k + " = " + v)
        for n in self.nodes:
            n.render(depth + 1, out)
        out.append(ind + "}")
        return out

    def save_text(self):
        """ConfigNode.Save without the wrapper: the values then the child nodes, CRLF"""
        out = [k + " = " + v for k, v in self.values]
        for n in self.nodes:
            n.render(0, out)
        return "\r\n".join(out) + "\r\n"


# ---------------------------------------------------------------------------------------------------------------- versions
def check_version(vmaj, vmin, vrev, lmaj, lmin, lrev):
    flag = False
    if vmaj > lmaj:
        flag = True
    elif vmaj == lmaj:
        if vmin > lmin:
            flag = True
        elif vmin == lmin and vrev >= lrev:
            flag = True
    if vmaj <= VER[0] and (vmaj != VER[0] or vmin <= VER[1]) and (vmaj != VER[0] or vmin != VER[1] or vrev <= VER[2]):
        return "COMPATIBLE" if flag else "TOO_EARLY"
    return "TOO_LATE"


class Compat:
    def __init__(self):
        self.maj = self.min = self.rev = INT_MAX

    def old_career_save_i(self, mode, a, b, c):
        self.maj, self.min, self.rev = a, b, c
        if mode != "CAREER":
            return False
        if VER[0] > a:
            return True
        if VER[0] == a:
            if VER[1] > b:
                return True
            if VER[1] == b and VER[2] > c:
                return True
        return False

    def old_career_save(self, mode, s):
        parts = s.split(".")
        if len(parts) != 3:
            return True
        vals = []
        for p in parts:
            m = re.fullmatch(r"\s*[+-]?[0-9]+\s*", p)
            if not m:
                return True
            vals.append(int(p))
        return self.old_career_save_i(mode, *vals)

    def prior_to(self, a, b, c):
        return check_version(self.maj, self.min, self.rev, a, b, c) == "TOO_EARLY"

    def upgrade_situation(self, s):
        if not re.fullmatch(r"\s*[+-]?[0-9]+\s*", s):
            return s
        r = int(s)
        if self.prior_to(1, 0, 5):
            m = {0: "LANDED", 1: "SPLASHED", 2: "PRELAUNCH", 3: "FLYING", 4: "SUB_ORBITAL", 5: "ORBITING", 6: "ESCAPING", 7: "DOCKED"}
            if r in m:
                return m[r]
        elif self.prior_to(1, 0, 6):
            m = {32: "ORBITING", 0: "LANDED", 1: "LANDED", 2: "SPLASHED", 3: "FLYING", 4: "PRELAUNCH", 5: "ORBITING", 6: "ESCAPING", 7: "DOCKED",
                 8: "FLYING", 16: "SUB_ORBITAL", 128: "DOCKED", 64: "ESCAPING"}
            if r in m:
                return m[r]
        return s


# ---------------------------------------------------------------------------------------------------------------- game data the upgrade reads
def part_titles():
    out = {}
    root = os.path.join(w37.SQUAD, "Parts")
    for dp, _dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(".cfg"):
                try:
                    n = w37.parse_cfg(os.path.join(dp, f))
                except Exception:
                    continue
                for pn in n["nodes"]:
                    if pn["name"] == "PART":
                        nm = w37.val(pn, "name")
                        t = w37.val(pn, "title")
                        if nm:
                            out[nm.replace("_", ".")] = t
    return out


PART_TITLES = part_titles()


def survey_defs():
    cfg = rcd.get_node(rcd.parse_cfg(os.path.join(w37.SQUAD, "Contracts", "Contracts.cfg")), "Contracts")
    survey = rcd.get_node(cfg, "Survey")
    out = []  # (data, anomaly, result, [experiments])
    for d in survey.nodes:
        if d.name != "SURVEY_DEFINITION":
            continue
        vals = {k: v for k, v, _ in d.values}
        exps = [{k: v for k, v, _ in p.values}.get("Experiment") for p in d.nodes if p.name == "PARAM"]
        # GameDatabase nodes are localized when the cfg is loaded (Localizer.TranslateBranch): the #autoLOC_ values are the localized strings
        tr = lambda x: LOC.fmt(x) if x.startswith("#") else x
        out.append((tr(vals.get("DataName", "generic")), tr(vals.get("AnomalyName", "anomaly")), tr(vals.get("ResultName", "report")), exps))
    return out


SURVEYS = survey_defs()


def find_survey(experiment):
    for d in reversed(SURVEYS):
        for e in reversed(d[3]):
            if e == experiment:
                return d
    return None


def body_by_index(i):
    return w37.BODY_LIST[i]


def is_vowel(c):
    return c.lower() in "aeiouy"


# ---------------------------------------------------------------------------------------------------------------- UpdateCareerSave
def update_career_save(cu, node):
    if node.get_value("name") != "ContractSystem":
        return
    node2 = node.get_node("CONTRACTS")
    if node2 is None:
        return
    flag = False
    count = len(node2.nodes)
    while count > 0:
        count -= 1
        cn = node2.nodes[count]
        if (cn.name != "CONTRACT" and cn.name != "CONTRACT_FINISHED") or not cn.has_value("type"):
            continue
        value = cn.get_value("type")
        if value in ("BaseContract", "StationContract"):
            if not cn.has_value("contextual"):
                cn.add_value("contextual", False)
                flag = True
        elif value == "PartTest":
            if not cn.has_value("haul"):
                cn.add_value("haul", False)
                flag = True
        elif value == "SurveyContract":
            if not cn.has_value("contextual"):
                cn.add_value("contextual", False)
                flag = True
            if not cn.has_value("targetBand"):
                cn.add_value("targetBand", "NONE")
                flag = True
            if not cn.has_value("locationName"):
                body = w38b.HOME
                tb = cn.get_value("targetBody")
                if tb is not None and re.fullmatch(r"\s*[+-]?[0-9]+\s*", tb):
                    body = body_by_index(int(tb))["name"]
                cn.add_value("locationName", w38b.BD_DISPLAY(body))
                flag = True
            if not cn.has_value("dataName") or not cn.has_value("anomalyName") or not cn.has_value("reportName"):
                sd = None
                c2 = len(cn.nodes)
                while c2 > 0:
                    c2 -= 1
                    c3 = cn.nodes[c2]
                    if c3.has_value("name") and c3.get_value("name") == "SurveyWaypointParameter":
                        if c3.has_value("experiment"):
                            sd = find_survey(c3.get_value("experiment"))
                        if sd is not None:
                            break
                if sd is not None:
                    if not cn.has_value("dataName"):
                        cn.add_value("dataName", sd[0])
                    if not cn.has_value("anomalyName"):
                        cn.add_value("anomalyName", sd[1])
                    if not cn.has_value("reportName"):
                        cn.add_value("reportName", sd[2])
                    flag = True
            cn.remove_values("surveyTitle")
            cn.remove_values("surveyBriefing")
            cn.remove_values("surveyDebriefing")
        elif value == "RecordTrackContract":
            node2.nodes.remove(cn)
            flag = True
        elif value == "ISRUContract":
            if cn.has_value("deliverySituation") and cu.prior_to(1, 0, 6):
                cn.set_value("deliverySituation", cu.upgrade_situation(cn.get_value("deliverySituation")))
                flag = True
        elif value == "SatelliteContract":
            if not cn.has_value("placementMode"):
                cn.add_value("placementMode", "0")
                flag = True
        elif value == "TourismContract":
            if not cn.has_value("preposition"):
                pres = {"Trivial": 0, "Significant": 1, "Exceptional": 2}.get(cn.get_value("prestige"), 0)
                text = ""
                if pres <= 1:
                    body = None
                    params = cn.get_nodes("PARAM")
                    tour = None
                    for pn in reversed(params):
                        if pn.has_value("name") and pn.get_value("name") == "KerbalTourParameter":
                            tour = pn
                            break
                    if tour is not None:
                        sub = tour.get_nodes("PARAM")
                        k = len(sub) - 1
                        while k >= 0:
                            if not sub[k].has_value("name") or sub[k].get_value("name") != "KerbalDestinationParameter":
                                k -= 1
                                continue
                            # sic: LoadNode(node, ...) reads the ContractSystem node, not the contract
                            tb = node.get_value("targetBody")
                            body = w38b.HOME
                            if tb is not None and re.fullmatch(r"\s*[+-]?[0-9]+\s*", tb):
                                body = body_by_index(int(tb))["name"]
                            break
                    if body is not None:
                        text = ("right here on " + w38b.BD_DISPLAY(body)) if w37.BODY[body]["home"] else ("around " + w38b.BD_DISPLAY(body))
                else:
                    text = "of the solar system"
                cn.add_value("preposition", text)
                flag = True
            if not cn.has_value("homeDestinations"):
                cn.add_value("homeDestinations", False)
                flag = True
            if cn.get_values("kerbalName"):
                lst = cn.get_values("kerbalName")
                if lst:
                    cn.add_value("tourists", "|".join(lst))
                cn.remove_values("kerbalName")
                flag = True
        for p in reversed(cn.get_nodes("PARAM")):
            if not p.has_value("name"):
                continue
            v6 = p.get_value("name")
            if v6 == "VesselSystemsParameter":
                if not p.has_value("checkModuleTypes") or not cu.prior_to(1, 1, 4):
                    continue
                arr = p.get_value("checkModuleTypes").split("|")
                arr = ["Generator" if a == "Power" else a for a in arr]
                p.set_value("checkModuleTypes", "|".join(arr))
                flag = True
            elif v6 == "SpecificOrbitParameter":
                if not p.has_value("TargetBody"):
                    tb = p.get_value("targetBody")
                    v15 = tb if tb is not None else "0"
                    p.remove_values("targetBody")
                    p.add_value("TargetBody", v15)
                    flag = True
            elif v6 in ("ProbeSystemsParameter", "FacilitySystemsParameter"):
                p.set_value("name", "VesselSystemsParameter", True)
                flag = True
                if not p.has_value("typeString"):
                    p.add_value("typeString", {"BaseContract": "outpost", "StationContract": "station", "SatelliteContract": "satellite"}.get(value, "vessel"))
                if not p.has_value("mannedStatus"):
                    p.add_value("mannedStatus", "UNMANNED" if v6 == "ProbeSystemsParameter" else "ANY")
                if not p.has_value("requireNew"):
                    p.add_value("requireNew", True)
                if not p.has_value("checkModuleTypes"):
                    p.add_value("checkModuleTypes", "Antenna|Power" if v6 == "ProbeSystemsParameter" else "Antenna|Power|Dock")
                if not p.has_value("checkModuleDescriptions"):
                    p.add_value("checkModuleDescriptions", "has an antenna|can generate power" if v6 == "ProbeSystemsParameter" else
                                "has an antenna|has a docking port|can generate power")
                p.remove_values("hasAntenna")
                p.remove_values("hasPowerGenerator")
                p.remove_values("hasDockingPort")
            elif v6 == "LocationAndSituationParameter":
                if p.has_value("targetSituation") and cu.prior_to(1, 0, 6):
                    p.set_value("targetSituation", cu.upgrade_situation(p.get_value("targetSituation")))
                    flag = True
            elif v6 == "SurveyWaypointParameter":
                if not p.has_value("contextual"):
                    p.add_value("contextual", False)
                    flag = True
            elif v6 == "PartTest":
                if not p.has_value("body"):
                    d = cn.get_value("dest")
                    if d is not None and re.fullmatch(r"\s*[+-]?[0-9]+\s*", d):
                        p.add_value("body", w37.BODY_LIST[int(d)]["name"])
                        flag = True
                if not p.has_value("situation"):
                    s = cn.get_value("sit")
                    p.add_value("situation", s if s is not None else "ORBITING")
                    flag = True
                if not p.has_value("uniqueID"):
                    sd = cn.get_value("seed")
                    seed = int(sd) if sd is not None and re.fullmatch(r"\s*[+-]?[0-9]+\s*", sd) else 0
                    p.add_value("uniqueID", rcd.g17(w38.KSPRandom(seed).next_double()))
                    flag = True
                if not p.has_value("haul"):
                    p.add_value("haul", False)
                    flag = True
                if not p.has_value("repeatability"):
                    p.add_value("repeatability", "ONCEPERPART")
                    flag = True
            elif v6 == "RecordTrackingParameter":
                cn.nodes.remove(p)
                flag = True
            elif v6 == "ResourceExtractionParameter":
                if not p.has_value("modules"):
                    p.add_value("modules", "ModuleResourceHarvester")
                    flag = True
            elif v6 == "StationaryPointParameter":
                if not p.has_value("deviation"):
                    p.add_value("deviation", "12.5")
                    flag = True
            elif v6 == "PartRequestParameter":
                if p.has_value("partNames"):
                    v7 = p.get_value("partNames")
                    if "," in v7:
                        p.remove_values("partNames")
                        p.add_value("partNames", "|".join(v7.split(",")))
                        flag = True
                if p.has_value("moduleNames"):
                    v8 = p.get_value("moduleNames")
                    if "," in v8:
                        p.remove_values("moduleNames")
                        p.add_value("moduleNames", "|".join(v8.split(",")))
                        flag = True
                if not p.has_value("partDescription"):
                    v9 = "part"
                    pn_, mn_ = p.get_value("partNames"), p.get_value("moduleNames")
                    if pn_ is None or mn_ is None:
                        raise KeyError("NullReferenceException")
                    for nm in pn_.split("|"):
                        key = nm.replace("_", ".")
                        if key in PART_TITLES:
                            t = PART_TITLES[key]
                            v9 = LOC.fmt(t) if t and t.startswith("#") else t
                            break
                    p.add_value("partDescription", v9)
                if not p.has_value("article"):
                    v13 = p.get_value("partDescription")
                    p.add_value("article", "an" if (len(v13) > 0 and is_vowel(v13[0])) else "a")
                if not p.has_value("vesselDescription"):
                    p.add_value("vesselDescription", {"SatelliteContract": "satellite", "BaseContract": "outpost", "StationContract": "station"}.get(value, "vessel"))
                p.remove_values("title")


def clean_evas(fs):
    num = int(fs.get_value("activeVessel"))
    nodes = fs.get_nodes("VESSEL")
    n2 = len(nodes)
    while n2 > 0:
        n2 -= 1
        cn = nodes[n2]
        if cn.get_value("type") != "EVA":
            continue
        for p in cn.get_nodes("PART"):
            if p.get_value("name") == "kerbalEVA" and p.get_value("crew") is None:
                fs.nodes.remove(cn)
                if n2 > num:
                    num -= 1
                break
    fs.set_value("activeVessel", str(num))


# ---------------------------------------------------------------------------------------------------------------- corpus
def P(name, params=None, **kv):
    vals = [["name", name]] + [[k, str(v)] for k, v in kv.items()]
    return N("PARAM", vals, params or [])


def C(ctype, params=None, finished=False, **kv):
    vals = [["type", ctype]] + [[k, str(v)] for k, v in kv.items()]
    return N("CONTRACT_FINISHED" if finished else "CONTRACT", vals, params or [])


def CS(*contracts, top=None, name="ContractSystem"):
    vals = [["name", name], ["scene", "7, 8, 5"]] + [[k, str(v)] for k, v in (top or {}).items()]
    return N("SCENARIO_X", vals, [N("CONTRACTS", [], list(contracts))])


def corpus():
    out = []

    def add(label, node):
        out.append((label, node))

    for t in ("BaseContract", "StationContract", "PartTest", "SatelliteContract", "ARMContract", "ISRUContract"):
        add(t + " bare", CS(C(t)))
        add(t + " finished", CS(C(t, finished=True)))
    add("contextual present", CS(C("BaseContract", contextual="True"), C("StationContract", contextual="False")))
    add("PartTest haul present", CS(C("PartTest", haul="True")))
    add("Satellite placement present", CS(C("SatelliteContract", placementMode="2")))
    add("RecordTrackContract", CS(C("ARMContract"), C("RecordTrackContract"), C("BaseContract")))
    add("no type", N("SCENARIO_X", [["name", "ContractSystem"]], [N("CONTRACTS", [], [N("CONTRACT", [["x", "1"]], [])])]))
    add("other node", CS(N("OTHER", [["type", "BaseContract"]], [])))
    add("not ContractSystem", CS(C("BaseContract"), name="Funding"))
    add("no CONTRACTS", N("SCENARIO_X", [["name", "ContractSystem"]], []))
    add("no name", N("SCENARIO_X", [], [N("CONTRACTS", [], [C("BaseContract")])]))
    # ISRU delivery situations
    for s in ("0", "1", "2", "3", "4", "5", "6", "7", "8", "16", "32", "64", "128", "9", "LANDED", "-1"):
        add("ISRU sit " + s, CS(C("ISRUContract", deliverySituation=s)))
        add("LocationAndSituation sit " + s, CS(C("ARMContract", [P("LocationAndSituationParameter", targetSituation=s)])))
    # surveys
    for exp in ("evaReport", "crewReport", "surfaceSample", "seismicScan", "temperatureScan", "barometerScan", "atmosphereAnalysis", "gravityScan", "nothing", None):
        params = [P("SurveyWaypointParameter")] if exp is None else [P("OtherParameter"), P("SurveyWaypointParameter", experiment=exp)]
        add("Survey exp %s" % exp, CS(C("SurveyContract", params, targetBody=3, surveyTitle="t", surveyBriefing="b", surveyDebriefing="d")))
    add("Survey complete", CS(C("SurveyContract", [P("SurveyWaypointParameter", contextual="True", experiment="evaReport")], contextual="True",
                                targetBand="LOW", locationName="Here", dataName="d", anomalyName="a", reportName="r")))
    add("Survey partial names", CS(C("SurveyContract", [P("SurveyWaypointParameter", experiment="evaReport")], dataName="d")))
    add("Survey bad body", CS(C("SurveyContract", [], targetBody="x")))
    add("Survey two params", CS(C("SurveyContract", [P("SurveyWaypointParameter", experiment="gravityScan"), P("SurveyWaypointParameter", experiment="evaReport")])))
    # tourism
    tour_kids = [P("KerbalDestinationParameter")]
    for pres in ("Trivial", "Significant", "Exceptional", "Bogus", None):
        for kids in (None, [], tour_kids, [P("Other")]):
            params = [] if kids is None else [P("KerbalTourParameter", kids)]
            for top in (None, 3, 1):
                kv = {} if pres is None else {"prestige": pres}
                add("Tourism %s %s top %s" % (pres, "none" if kids is None else len(kids), top),
                    CS(C("TourismContract", params, **kv), top=({} if top is None else {"targetBody": top})))
    add("Tourism names", CS(C("TourismContract", [], preposition="there", homeDestinations="True", prestige="Trivial")))
    add("Tourism kerbalName", N("SCENARIO_X", [["name", "ContractSystem"]], [N("CONTRACTS", [], [N("CONTRACT", [["type", "TourismContract"], ["kerbalName", "A Kerman"],
                                                                                                                ["kerbalName", "B Kerman"], ["preposition", "x"],
                                                                                                                ["homeDestinations", "False"]], [])])]))
    add("Tourism one name", N("SCENARIO_X", [["name", "ContractSystem"]], [N("CONTRACTS", [], [N("CONTRACT", [["type", "TourismContract"], ["kerbalName", "Only Kerman"],
                                                                                                              ["preposition", "x"], ["homeDestinations", "False"]], [])])]))
    # parameters
    for cm in ("Dock|Power|Antenna", "Power", "Antenna|Other", "Generator|Dock", ""):
        add("VesselSystems types " + cm, CS(C("BaseContract", [P("VesselSystemsParameter", checkModuleTypes=cm)], contextual="False")))
    add("VesselSystems no types", CS(C("BaseContract", [P("VesselSystemsParameter")], contextual="False")))
    for tb in (None, "4", "0"):
        add("SpecificOrbit TargetBody %s" % tb, CS(C("SatelliteContract", [P("SpecificOrbitParameter", **({} if tb is None else {"targetBody": tb}))], placementMode="0")))
    add("SpecificOrbit has TargetBody", CS(C("SatelliteContract", [P("SpecificOrbitParameter", TargetBody="2", targetBody="9")], placementMode="0")))
    for pname in ("ProbeSystemsParameter", "FacilitySystemsParameter"):
        for ct in ("BaseContract", "StationContract", "SatelliteContract", "ARMContract"):
            add("%s in %s" % (pname, ct), CS(C(ct, [P(pname, hasAntenna="True", hasPowerGenerator="False", hasDockingPort="True")], contextual="False", placementMode="0")))
        add("%s filled" % pname, CS(C("ARMContract", [P(pname, typeString="rover", mannedStatus="MANNED", requireNew="False", checkModuleTypes="Dock",
                                                         checkModuleDescriptions="x")])))
    add("SurveyWaypoint contextual", CS(C("ARMContract", [P("SurveyWaypointParameter"), P("SurveyWaypointParameter", contextual="True")])))
    # PartTest parameter
    for dest in (None, "3", "99x"):
        for sit in (None, "FLYING"):
            for seed in (None, "12345", "bad"):
                for pre in (False, True):
                    kv = {}
                    if dest is not None:
                        kv["dest"] = dest
                    if sit is not None:
                        kv["sit"] = sit
                    if seed is not None:
                        kv["seed"] = seed
                    pkv = {"body": "Kerbin", "situation": "LANDED", "uniqueID": "0.5", "haul": "True", "repeatability": "ALWAYS"} if pre else {}
                    add("PartTest param dest=%s sit=%s seed=%s pre=%s" % (dest, sit, seed, pre), CS(C("ARMContract", [P("PartTest", **pkv)], **kv)))
    add("RecordTrackingParameter", CS(C("ARMContract", [P("A"), P("RecordTrackingParameter"), P("B")])))
    add("ResourceExtraction", CS(C("ISRUContract", [P("ResourceExtractionParameter"), P("ResourceExtractionParameter", modules="ModuleX")], deliverySituation="LANDED")))
    add("StationaryPoint", CS(C("SatelliteContract", [P("StationaryPointParameter"), P("StationaryPointParameter", deviation="3")], placementMode="0")))
    # PartRequestParameter
    for pn_, mn_ in (("HighGainAntenna", "ModuleNotExist"), ("mk1pod_v2", "ModuleNotExist"), ("solarPanels1", ""), ("Large_Crewed_Lab", "ModuleNotExist"),
                     ("noSuchPart", "ModuleNotExist"), ("noSuchPart,HighGainAntenna", "ModuleNotExist"), ("HighGainAntenna", "ModuleA,ModuleB"),
                     ("InfraredTelescope", "x"), ("probeCoreOcto_v2", "x"), ("liquidEngine", "x")):
        for ct in ("BaseContract", "SatelliteContract", "StationContract", "ARMContract"):
            add("PartRequest %s/%s in %s" % (pn_, mn_, ct), CS(C(ct, [P("PartRequestParameter", partNames=pn_, moduleNames=mn_, title="old")], contextual="False", placementMode="0")))
    add("PartRequest filled", CS(C("ARMContract", [P("PartRequestParameter", partNames="A|B", moduleNames="C", partDescription="engine", article="an", vesselDescription="rocket")])))
    add("PartRequest description only", CS(C("ARMContract", [P("PartRequestParameter", partNames="HighGainAntenna", moduleNames="x", partDescription="Umbrella")])))
    add("PartRequest empty description", CS(C("ARMContract", [P("PartRequestParameter", partNames="HighGainAntenna", moduleNames="x", partDescription="")])))
    # mixed multi-contract
    add("mixed", CS(C("BaseContract", [P("ProbeSystemsParameter"), P("PartRequestParameter", partNames="HighGainAntenna", moduleNames="x")]),
                    C("SurveyContract", [P("SurveyWaypointParameter", experiment="evaReport")]),
                    C("RecordTrackContract"), C("TourismContract", [P("KerbalTourParameter", [P("KerbalDestinationParameter")])], prestige="Trivial"),
                    C("ISRUContract", [], deliverySituation="3"), C("SatelliteContract"), C("PartTest", [P("PartTest")], finished=True)))
    return out


VERSIONS = ["0.90.0", "1.0.4", "1.0.5", "1.0.6", "1.1.3", "1.1.4", "1.2.0", "1.12.4", "1.12.5", "2.0.0", "1.12", "1.x.5", "a.b.c", "1.0.4.1", " 1 . 0 . 4 ", "1.0.-1"]


def eva_cases():
    out = []

    def vessel(t, parts):
        return N("VESSEL", [["type", t]], [N("PART", ([["name", nm]] + ([["crew", c]] if c else [])), []) for nm, c in parts])

    out.append(("eva-mixed", N("FLIGHTSTATE", [["activeVessel", "2"]], [vessel("Ship", [("mk1pod", "Jeb")]), vessel("EVA", [("kerbalEVA", None)]), vessel("EVA", [("kerbalEVA", "Bob")]),
                                                                        vessel("EVA", [("kerbalEVA", None)]), vessel("Probe", [("probeCoreOcto", None)])])))
    out.append(("eva-active-last", N("FLIGHTSTATE", [["activeVessel", "4"]], [vessel("EVA", [("kerbalEVA", None)]), vessel("Ship", [("a", None)]),
                                                                             vessel("EVA", [("kerbalEVA", None)]), vessel("Ship", [("b", None)]), vessel("Ship", [("c", None)])])))
    out.append(("eva-active-0", N("FLIGHTSTATE", [["activeVessel", "0"]], [vessel("EVA", [("kerbalEVA", None)]), vessel("EVA", [("kerbalEVA", None)])])))
    out.append(("eva-none", N("FLIGHTSTATE", [["activeVessel", "1"]], [vessel("Ship", [("a", "x")])])))
    out.append(("eva-no-vessels", N("FLIGHTSTATE", [["activeVessel", "7"]], [])))
    out.append(("eva-other-part-names", N("FLIGHTSTATE", [["activeVessel", "1"]], [vessel("EVA", [("kerbalEVA2", None), ("x", None)]), vessel("EVA", [("x", None), ("kerbalEVA", None)])])))
    return out


def build(cs_root, mirror):
    out = {"versions": VERSIONS, "cases": [], "eva": []}
    for label, node in corpus():
        for ver in VERSIONS:
            cu = Compat()
            old = cu.old_career_save("CAREER", ver)
            res = {"label": label, "version": ver, "old": old, "input": node.to_json()}
            n2 = N.from_json(node.to_json())
            try:
                if old:
                    update_career_save(cu, n2)
                res["output"] = n2.save_text()
            except KeyError:
                res["output"] = None  # NullReferenceException
            out["cases"].append(res)
    # OldCareerSave table
    tab = []
    for mode in ("CAREER", "SANDBOX", "SCIENCE_SANDBOX"):
        for ver in VERSIONS + ["1.12.6", "1.13.0", "1.12.5", "1.11.9", "0.0.0", "", "...", "1.0"]:
            cu = Compat()
            tab.append({"mode": mode, "version": ver, "old": cu.old_career_save(mode, ver), "ints": [cu.maj, cu.min, cu.rev]})
    out["oldTable"] = tab
    sit = []
    for ver in ("0.90.0", "1.0.4", "1.0.5", "1.0.6", "1.1.0"):
        for s in [str(i) for i in range(-2, 140)] + ["LANDED", "", "x1"]:
            cu = Compat()
            cu.old_career_save_i("CAREER", *[int(x) for x in ver.split(".")])
            sit.append({"version": ver, "in": s, "out": cu.upgrade_situation(s)})
    out["situations"] = sit
    pr = []
    for ver in ("0.90.0", "1.0.5", "1.1.3", "1.1.4", "1.12.5", "1.13.0"):
        cu = Compat()
        cu.old_career_save_i("CAREER", *[int(x) for x in ver.split(".")])
        pr.append({"version": ver, "p": [[a, b, c, cu.prior_to(a, b, c)] for (a, b, c) in ((1, 0, 5), (1, 0, 6), (1, 1, 4), (0, 90, 0), (1, 12, 5), (2, 0, 0))]})
    out["priorTo"] = pr
    for label, node in eva_cases():
        n2 = N.from_json(node.to_json())
        clean_evas(n2)
        out["eva"].append({"label": label, "input": node.to_json(), "output": n2.save_text()})
    print("compat cases: %d (%d with a changed save)" % (len(out["cases"]), sum(1 for c in out["cases"] if c["output"] != N.from_json(c["input"]).save_text())))
    return out
