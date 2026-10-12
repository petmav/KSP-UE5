"""WP-54 slice 2 reference, part 5: the Kerbal family (Expansions/Missions/Tests/TestKerbal*.cs, TestTakeKerbal.cs).
Dynamic models: TestKerbalKilled, TestKerbalRecovery, TestKerbalRescue. Static (title / info / Save / body text): TestKerbalEVA, TestKerbalBoarded, TestTakeKerbal.
No kspcore code."""
from ref_common import *  # noqa: F401,F403
from ref_common import Cfg, fmt, b
from ref_data import rng_pick
from ref_tests2 import TM, gen_world, active, vtext, gen_surf_cfg, Surf, KERBALS
from ref_tests3 import MK, gen_mk_cfg, head_ops, body_ops, new_env, crew_of, parse_bool, ROSTER_TOURISTS

STATUSES = ["Available", "Assigned", "Dead", "Missing"]


def type_of(name):
    return "Tourist" if name in ROSTER_TOURISTS else "Crew"


class Roster:
    """the mission roster (Fixture: five crew and two tourists, all Available at the start of a case)"""

    def __init__(self):
        self.status = {n: "Available" for n in KERBALS + ROSTER_TOURISTS}


class KerbalTM(TM):
    TITLE_RAW = False

    def __init__(self, roster):
        super().__init__()
        self.mk = MK("Crew", True)
        self.roster = roster
        self.found = False
        self.sub = False

    def load(self, cfg):
        self.load_base(cfg)
        self.mk.load(cfg)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.mk.save(out)
        return out

    def body_text(self, f):
        if f["name"] == "missionKerbal":
            return self.mk.text()
        return self.base_body(f)

    def initialized(self):
        pass


class Killed(KerbalTM):
    NAME = "TestKerbalKilled"
    TITLE = "#autoLOC_8007400"
    INFO = "#autoLOC_8007401"

    def initialized(self):
        self.found = False
        self.sub = True
        if self.mk.kerbal is not None:
            name, typ = self.mk.kerbal
            if self.mk.type == typ and self.roster.status[name] in ("Missing", "Dead"):
                self.found = True

    def killed(self, sender):
        if not self.sub:
            return
        if self.mk.kerbal is None:
            if type_of(sender) == self.mk.type:
                self.found = True
        elif sender == self.mk.kerbal[0]:
            self.found = True


class Recovery(KerbalTM):
    NAME = "TestKerbalRecovery"
    TITLE = "#autoLOC_8000150"
    INFO = "#autoLOC_8004018"

    def initialized(self):
        self.found = False
        if self.mk.kerbal is not None and self.roster.status[self.mk.kerbal[0]] == "Available":
            self.found = True
        if not self.found:
            self.sub = True

    def recovered(self, crew):
        if not self.sub:
            return
        if not crew:
            return
        if self.mk.kerbal is not None:
            for n in reversed(crew):
                if self.mk.is_valid(n, type_of(n)):
                    self.found = True
        else:
            self.found = True


class Rescue(KerbalTM):
    NAME = "TestKerbalRescue"
    TITLE = "#autoLOC_8000154"
    INFO = "#autoLOC_8004019"

    def __init__(self, roster):
        super().__init__(roster)
        self.must_board = False
        self.sub_board = False
        self.sub_know = False

    def load(self, cfg):
        super().load(cfg)
        v = cfg.get("mustBoardVessel")
        if v is not None:
            self.must_board = parse_bool(v)

    def save(self):
        out = super().save()
        out.values.append(("mustBoardVessel", b(self.must_board)))
        return out

    def initialized_w(self, w):
        self.found = False
        for v in reversed(w["vessels"]):
            for n in crew_of(v):
                if self.mk.is_valid(n, type_of(n)) and v["disc"] == "Owned":
                    if not self.must_board or (self.must_board and v["type"] != "EVA"):
                        self.found = True
                    break
        if not self.found:
            if self.must_board:
                self.sub_board = True
            else:
                self.sub_know = True

    def knowledge(self, v, to):
        if not self.sub_know:
            return
        for n in crew_of(v):
            if self.mk.kerbal is None:
                raise KeyError("NRE")
            if n == self.mk.kerbal[0] and to == "Owned":
                self.found = True

    def transferred(self, name):
        if not self.sub_board:
            return
        if self.mk.kerbal is None or self.mk.kerbal[0] == name:
            self.found = True


class Static(TM):
    """EVA / Boarded / TakeKerbal: Save and the body texts (the event logic is not gated)"""

    def __init__(self, any_body_type):
        super().__init__()
        self.mk = MK(any_body_type, True)
        self.s = Surf(False)
        self.vesselID = 0

    def load(self, cfg):
        self.load_base(cfg)
        self.mk.load(cfg)
        self.s.load(cfg)
        v = cfg.get("vesselID")
        if v is not None:
            self.vesselID = int(v)

    def save(self):
        out = Cfg("N")
        self.save_base(out)
        self.mk.save(out)
        self.s.save(out)
        out.values.append(("vesselID", str(self.vesselID)))
        return out

    def body_text(self, f):
        if f["name"] == "missionKerbal":
            return self.mk.text()
        if f["name"] == "vesselID":
            return fmt("#autoLOC_8000069", fmt("#autoLOC_8001004"))
        return self.base_body(f)


class EVA(Static):
    NAME = "TestKerbalEVA"
    TITLE = "#autoLOC_8000185"
    INFO = "#autoLOC_8004014"


class Boarded(Static):
    NAME = "TestKerbalBoarded"
    TITLE = "#autoLOC_8000148"
    INFO = "#autoLOC_8004017"


class Take(Static):
    NAME = "TestTakeKerbal"
    TITLE = "#autoLOC_8000174"
    INFO = "#autoLOC_8004033"


MK_FIELD = [{"name": "missionKerbal", "guiName": "#autoLOC_8007219"}]
VESSEL_FIELD = [{"name": "vesselID", "guiName": "#autoLOC_8000001"}]


def roster_ops(r, m, ops, rost):
    if r.random() < 0.7:
        n = rng_pick(r, KERBALS + ROSTER_TOURISTS)
        s = rng_pick(r, STATUSES)
        ops.append({"op": "roster", "name": n, "status": s})
        rost.status[n] = s


def gen_killed(r):
    rost = Roster()
    m = Killed(rost)
    cfg = Cfg("N")
    gen_mk_cfg(r, cfg)
    if r.random() < 0.3:
        cfg.values.append(("title", "Dead"))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, MK_FIELD)
    for step in range(6):
        roster_ops(r, m, ops, rost)
        w = gen_world(r)
        new_env(w, ops, 100.0)
        if r.random() < 0.6:
            ops.append({"op": "init"})
            m.initialized()
        ops.append({"op": "test", "out": m.found})
        for k in range(r.randrange(0, 3)):
            sender = rng_pick(r, KERBALS + ROSTER_TOURISTS)
            ops.append({"op": "event", "name": "crewKilled", "sender": sender})
            m.killed(sender)
            ops.append({"op": "test", "out": m.found})
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "roster": True}


def gen_recovery(r):
    rost = Roster()
    m = Recovery(rost)
    cfg = Cfg("N")
    gen_mk_cfg(r, cfg)
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, MK_FIELD)
    for step in range(6):
        roster_ops(r, m, ops, rost)
        w = gen_world(r)
        new_env(w, ops, 100.0)
        if r.random() < 0.7:
            ops.append({"op": "init"})
            m.initialized()
        ops.append({"op": "test", "out": m.found})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, active(w)), "number": None}})
        for k in range(r.randrange(0, 3)):
            crew = r.sample(KERBALS + ROSTER_TOURISTS, r.randrange(0, 4))
            ops.append({"op": "event", "name": "vesselRecovered", "crew": crew})
            m.recovered(crew)
            ops.append({"op": "test", "out": m.found})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "roster": True}


def gen_rescue(r):
    rost = Roster()
    m = Rescue(rost)
    cfg = Cfg("N")
    gen_mk_cfg(r, cfg)
    cfg.values.append(("mustBoardVessel", rng_pick(r, ["True", "False", "False"])))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, MK_FIELD)
    for step in range(5):
        w = gen_world(r)
        new_env(w, ops, 100.0)
        if r.random() < 0.8:
            ops.append({"op": "init"})
            m.initialized_w(w)
        ops.append({"op": "test", "out": m.found})
        ops.append({"op": "scoreMod", "module": "ScoreModule_Resource", "out": {"vessel": vtext(w, active(w)), "number": None}})
        for k in range(r.randrange(0, 3)):
            if r.random() < 0.5 and w["vessels"]:
                v = rng_pick(r, w["vessels"])
                to = rng_pick(r, ["Owned", "Unowned"])
                if m.mk.kerbal is None and crew_of(v):
                    continue  # the C# throws an NRE inside the event handler; not part of the gate
                ops.append({"op": "event", "name": "knowledgeChanged", "vessel": v["pid"], "to": to})
                m.knowledge(v, to)
            else:
                name = rng_pick(r, KERBALS + ROSTER_TOURISTS)
                ops.append({"op": "event", "name": "crewTransferred", "sender": name})
                m.transferred(name)
            ops.append({"op": "test", "out": m.found})
    ops.append({"op": "saved", "out": m.save().dump()})
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "roster": True}


def gen_static(r, cls, kt):
    m = cls(kt)
    cfg = gen_surf_cfg(r)
    gen_mk_cfg(r, cfg)
    cfg.values.append(("vesselID", rng_pick(r, ["0", "107"])))
    if r.random() < 0.3:
        cfg.values.append(("title", "Custom"))
    m.load(cfg)
    ops = head_ops(m) + body_ops(m, MK_FIELD + VESSEL_FIELD)
    return {"kind": "test", "cls": m.NAME, "cfg": cfg.to_json(), "ops": ops, "roster": True}


def generate(r):
    cases = []
    for _ in range(30):
        cases.append(gen_killed(r))
        cases.append(gen_recovery(r))
        cases.append(gen_rescue(r))
    for _ in range(10):
        cases.append(gen_static(r, EVA, "Crew"))
        cases.append(gen_static(r, Boarded, "Crew"))
        cases.append(gen_static(r, Take, "Tourist"))
    return cases
