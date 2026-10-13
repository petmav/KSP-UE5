"""WP-54c reference, part 4: MissionScoreInfo, MissionLocalizer, MissionImporting, MissionEditorHistory (Expansions/Missions/{MissionScoreInfo,MissionLocalizer,
MissionImporting}.cs, Editor/MissionEditorHistory.cs). Written from the C#; the file system, the zip archives, the popups, the clock and the localization files are
in-memory worlds on both sides (labelled synthetic). No kspcore code."""
import copy

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import Cfg, fmt, guid_of, rng_pick, D, F, f32, EMPTY_GUID

CFG_SENTINEL = None


# ======================================================================================================================================
# MissionScoreInfo
# ======================================================================================================================================
class SI:
    def __init__(self, gen):
        self.id = gen.next()
        self.mission = EMPTY_GUID
        self.score = 0.0
        self.time = 0.0
        self.awards = []
        self.text = ""
        self.completed = None

    def load(self, cfg):
        v = cfg.get("id")
        if v is not None:
            self.id = v
        v = cfg.get("missionId")
        if v is not None:
            self.mission = v
        v = cfg.get("score")
        if v is not None:
            self.score = f32(float(v))
        v = cfg.get("timeTaken")
        if v is not None:
            self.time = float(v)
        v = cfg.get("resultsText")
        if v is not None:
            self.text = v           # no unescape in ScoreInfo.Load
        self.completed = None
        v = cfg.get("completedAt")
        if v is not None:
            self.completed = int(v)
        self.awards = cfg.get_all("awardId")

    def save(self):
        c = Cfg("SCOREINFO")
        c.values.append(("id", self.id))
        c.values.append(("missionId", self.mission))
        c.values.append(("score", F(self.score)))
        c.values.append(("timeTaken", D(self.time)))
        c.values.append(("resultsText", self.text.replace(chr(10), chr(92) + "n").replace(chr(9), chr(92) + "t")))
        if self.completed is not None:
            c.values.append(("completedAt", str(self.completed)))
        for a in self.awards:
            c.values.append(("awardId", a))
        return c

    def view(self):
        return [self.id, self.mission, self.score, self.time, self.awards, self.completed]


class GuidGen:
    def __init__(self, start):
        self.n = start

    def next(self):
        g = guid_of(self.n)
        self.n += 1
        return g


class ScoreInfoModel:
    def __init__(self, gen):
        self.gen = gen
        self.d = {}      # mission guid -> [SI] (insertion ordered)

    def add_score(self, mission, score, ut, awards, enabled, now):
        if enabled:
            si = SI(self.gen)
            si.mission = mission
            si.score = f32(score)
            si.time = ut
            si.awards = list(awards)
            si.completed = now
            si.text = "#autoLOC_8100310"       # Mission.PrintObjectives of a mission without nodes returns this tag untranslated
            lst = self.d.setdefault(mission, [])
            lst.append(si)
            lst.sort(key=lambda x: (-x.score, x.time))     # stable; CompareScoreInfo (the scenarios keep (score, time) unique)

    def delete_score(self, sid):
        for k, lst in self.d.items():
            for si in lst:
                if si.id == sid:
                    lst.remove(si)
                    return

    def delete_mission(self, mission):
        self.d.pop(mission, None)

    def scores(self, mission):
        return [s.view() for s in self.d.get(mission, [])]

    def score(self, mission, idx):
        lst = self.d.get(mission)
        if lst is not None and idx < len(lst):
            return lst[idx].id
        return None

    def awards(self, mission):
        out = []
        for s in self.d.get(mission, []):
            for a in s.awards:
                if a not in out:
                    out.append(a)
        return out

    def load(self, cfg):
        for n in cfg.nodes:
            if n.name == "SCOREINFO":
                si = SI(self.gen)
                si.load(n)
                self.d.setdefault(si.mission, []).append(si)

    def save(self):
        out = Cfg("N")
        for k, lst in self.d.items():
            for si in lst:
                out.nodes.append(si.save())
        return out


MISSIONS = [guid_of(10), guid_of(11), guid_of(12)]
AWARD_POOL = ["Gold", "Silver", "Bronze", "Pioneer", "Survivor"]


def gen_scoreinfo(r):
    gen = GuidGen(1000)
    m = ScoreInfoModel(gen)
    spec = {"guid0": 1000}
    ops = []
    t = [0.0]
    used_delete_mission = False

    def fresh_cfg():
        cfg = Cfg("N")
        for _ in range(r.randrange(0, 4)):
            n = Cfg("SCOREINFO")
            n.values.append(("id", guid_of(2000 + r.randrange(0, 50))))
            n.values.append(("missionId", rng_pick(r, MISSIONS)))
            n.values.append(("score", rng_pick(r, ["10", "250.5", "0", "-5.25", "1000"])))
            n.values.append(("timeTaken", rng_pick(r, ["0", "100.5", "86400", "1234567.25"])))
            n.values.append(("resultsText", rng_pick(r, ["", "a" + chr(92) + "nb", "text"])))
            if r.random() < 0.7:
                n.values.append(("completedAt", rng_pick(r, ["132000000000000000", "133000000000000001", "116444736000000000"])))
            for a in r.sample(AWARD_POOL, r.randrange(0, 3)):
                n.values.append(("awardId", a))
            cfg.nodes.append(n)
        return cfg

    if r.random() < 0.6:
        cfg = fresh_cfg()
        ops.append({"op": "load", "cfg": cfg.to_json()})
        m.load(cfg)
        ops.append({"op": "save", "out": m.save().dump(), "guid": gen.n})
    for k in range(r.randrange(6, 12)):
        t[0] += 1.0 + r.randrange(0, 5)
        kind = rng_pick(r, ["add", "add", "add", "query", "delete", "deleteMission", "save", "file"])
        mission = rng_pick(r, MISSIONS)
        if kind == "add" and not used_delete_mission:
            score = rng_pick(r, [0.0, 10.0, 250.5, 1000.0, 77.75, 5.5])
            # keep (score, time) unique inside a mission: the time is a counter
            awards = r.sample(AWARD_POOL, r.randrange(0, 3))
            enabled = r.random() < 0.85
            now = rng_pick(r, [132000000000000000, 133000000000000001, 133111111111111111])
            m.add_score(mission, score, t[0], awards, enabled, now)
            ops.append({"op": "add", "mission": mission, "score": score, "ut": t[0], "awards": awards, "enabled": enabled, "now": now, "guid": gen.n,
                        "scores": m.scores(mission)})
        elif kind == "query":
            ops.append({"op": "query", "mission": mission, "scores": m.scores(mission), "score0": m.score(mission, 0), "score1": m.score(mission, 1), "score5": m.score(mission, 5),
                        "awards": m.awards(mission)})
        elif kind == "delete":
            all_ids = [s.id for lst in m.d.values() for s in lst]
            sid = rng_pick(r, all_ids + [guid_of(9999)])
            m.delete_score(sid)
            ops.append({"op": "delete", "id": sid, "dump": m.save().dump()})
        elif kind == "deleteMission" and not used_delete_mission:
            m.delete_mission(mission)
            used_delete_mission = True
            ops.append({"op": "deleteMission", "mission": mission, "dump": m.save().dump()})
        elif kind == "save":
            ops.append({"op": "save", "out": m.save().dump(), "guid": gen.n})
        elif kind == "file":
            root = Cfg("")
            ms = Cfg("MISSIONSCORES")
            ms.nodes = m.save().nodes
            root.nodes.append(ms)
            ops.append({"op": "saveFile", "out": root.dump(), "path": "Missions/MissionScoreInfo.cfg"})
            # LoadScores: a new instance loads what was saved
            m2 = ScoreInfoModel(gen)
            m2.load(ms)
            ops.append({"op": "loadFile", "dump": m2.save().dump(), "guid": gen.n})
            m = m2
    ops.append({"op": "nullMission"})
    return {"kind": "scoreinfo", "spec": spec, "ops": ops}


# ======================================================================================================================================
# MissionLocalizer
# ======================================================================================================================================
def gen_localizer(r):
    users = "ROOT/Missions/"
    dirs = set([users.rstrip("/")])
    files = {}
    names = []
    for i in range(r.randrange(0, 4)):
        d = users + "m%d" % i
        dirs.add(d)
        names.append(d)
        if r.random() < 0.7:
            dirs.add(d + "/Localization")
            for j in range(r.randrange(0, 3)):
                root = Cfg("")
                for k in range(r.randrange(0, 3)):
                    n = Cfg("Localization")
                    lang = n.nodes.append(Cfg(rng_pick(r, ["en-us", "es-es"])))
                    n.nodes[-1].values.append(("#autoLOC_m%d" % i, "text %d.%d.%d" % (i, j, k)))
                    root.nodes.append(n)
                if r.random() < 0.3:
                    root.nodes.append(Cfg("Other"))
                files[d + "/Localization/f%d.cfg" % j] = root
        if r.random() < 0.2:
            files[d + "/note.txt"] = Cfg("")
    out = []
    for d in sorted(n for n in dirs if n.startswith(users) and "/" not in n[len(users):]):
        if d + "/Localization" in dirs:
            for p in sorted(p for p in files if p.startswith(d + "/Localization/") and p.endswith(".cfg") and "/" not in p[len(d + "/Localization/"):]):
                for n in files[p].nodes:
                    if n.name == "Localization":
                        out.append(n.dump())
    spec = {"root": "ROOT/", "dirs": sorted(dirs), "files": {k: v.to_json() for k, v in files.items()}, "missionsExists": r.random() < 0.9}
    if not spec["missionsExists"]:
        out = []
    return {"kind": "localizer", "spec": spec, "out": out}


# ======================================================================================================================================
# MissionImporting
# ======================================================================================================================================
def stamp():
    return "2026_10_12_09_30_00"


class Importer:
    def __init__(self, w):
        self.users = "ROOT/Missions/"
        self.dirs = set(w["dirs"])
        self.files = set(w["files"])
        self.zips = w["zips"]
        self.move_fails = set(w.get("moveFails", []))
        self.msgs = []
        self.popups = []
        self.fires = 0
        self.parses = []
        self.log = []

    def msg(self, text, dur, style="UPPER_CENTER"):
        self.msgs.append([text, dur, style])

    def move_file(self, src, dst, overwrite):
        try:
            if src in self.move_fails or src not in self.files:
                raise IOError()
            if overwrite and dst in self.files:
                self.files.discard(dst)
            self.files.discard(src)
            self.files.add(dst)
        except IOError:
            return False
        return True

    def popup(self, name, message, title, buttons):
        self.popups.append({"name": name, "message": message, "title": title, "buttons": buttons})

    def move_imported(self, src, dst):
        if dst in self.files:
            text = dst[dst.rfind("/") + 1:]
            self.popup("ConfirmImportedFileOverwrite", fmt("#autoLOC_8005331", text), fmt("#autoLOC_8005332"),
                       [(fmt("#autoLOC_8005333"), ("move", src, dst)), (fmt("#autoLOC_8005334"), ("movedate", src, dst))])
        elif self.move_file(src, dst, True):
            self.msg(fmt("#autoLOC_8005329"), 5.0)
        else:
            self.msg(fmt("#autoLOC_8005330"), 10.0, "UPPER_LEFT")

    def delete_dir(self, d):
        self.dirs = set(x for x in self.dirs if not (x == d or x.startswith(d + "/")))
        self.files = set(x for x in self.files if not x.startswith(d + "/"))

    def confirmed(self, fp, dirs, dst):
        for d in dirs:
            if self.users + d in self.dirs:
                self.delete_dir(self.users + d)
        z = self.zips[fp]
        try:
            if z.get("throwDecompress"):
                raise IOError()
            for d in z["creates"]:
                self.dirs.add(self.users + d)
            self.msg(fmt("#autoLOC_8005328"), 5.0, "UPPER_LEFT")
            self.move_imported(fp, dst)
            self.parses.append(self.users)
            self.fires += 1
        except IOError:
            self.msg(fmt("#autoLOC_8005323"), 6.0, "UPPER_LEFT")
            self.move_imported(fp, dst)

    def import_zip(self, fp, dst, n):
        z = self.zips[fp]
        try:
            if z.get("throwRoot"):
                raise IOError()
            if z["rootFiles"]:
                self.msg(fmt("#autoLOC_8005323"), 6.0, "UPPER_LEFT")
                self.move_imported(fp, dst)
                return False
            if z.get("throwDirs"):
                raise IOError()
            dirs = list(z["dirs"])
            if len(dirs) < 1:
                self.msg(fmt("#autoLOC_8005323"), 6.0, "UPPER_LEFT")
                self.move_imported(fp, dst)
                return False
        except IOError:
            self.msg("<color=red>[MissionImporting]:Zip File Parsing Failed. Check the log.</color>", 6.0, "UPPER_LEFT")
            self.move_imported(fp, dst)
            return False
        text = ""
        for d in dirs:
            if self.users + d in self.dirs:
                text = text + chr(9) + d + chr(10)
        if text != "":
            text2 = fp[fp.rfind("/") + 1:]
            self.popup("ConfirmMissionOverwrite%d" % n, fmt("#autoLOC_8005324", text2, text), fmt("#autoLOC_8005325"),
                       [(fmt("#autoLOC_8005326"), ("confirm", fp, tuple(dirs), dst)), (fmt("#autoLOC_8005327"), ("move", fp, dst))])
        else:
            self.confirmed(fp, dirs, dst)
        return True

    def import_new(self):
        if self.users.rstrip("/") not in self.dirs:
            return
        zips = sorted(f for f in self.files if f.startswith(self.users) and "/" not in f[len(self.users):] and f.endswith(".zip"))
        if len(zips) >= 1:
            completed = self.users + "_Imported/"
            if completed.rstrip("/") not in self.dirs:
                self.dirs.add(completed.rstrip("/"))
            n = 0
            for t in zips:
                self.import_zip(t, completed + t[len(self.users):], n)
                n += 1

    def click(self, popup, idx):
        kind = popup["buttons"][idx][1]
        if kind[0] == "move":
            if kind[0] == "move" and len(kind) == 3 and popup["name"] == "ConfirmImportedFileOverwrite":
                self.move_file(kind[1], kind[2], True)
            else:
                self.move_imported(kind[1], kind[2])
        elif kind[0] == "movedate":
            dst = kind[2]
            dot = dst.rfind(".")
            self.move_file(kind[1], dst[:dot] + " (" + stamp() + ")" + dst[dot:], True)
        elif kind[0] == "confirm":
            self.confirmed(kind[1], list(kind[2]), kind[3])

    def snapshot(self):
        return {"files": sorted(self.files), "dirs": sorted(self.dirs), "msgs": self.msgs, "fires": self.fires, "parses": self.parses,
                "popups": [[p["name"], p["message"], p["title"], [b[0] for b in p["buttons"]]] for p in self.popups_log]}


def gen_importing(r):
    users = "ROOT/Missions/"
    files = set()
    zips = {}
    dirs = set(["ROOT/Missions"]) if r.random() < 0.9 else set()
    existing = ["Apollo", "Gemini", "Mercury"]
    for d in existing:
        if r.random() < 0.5:
            dirs.add(users + d)
    for i in range(r.randrange(0, 4)):
        p = users + "z%d.zip" % i
        files.add(p)
        zips[p] = {"rootFiles": r.random() < 0.15, "dirs": r.sample(existing + ["Voyager"], r.randrange(0, 3)), "throwRoot": r.random() < 0.08, "throwDirs": r.random() < 0.08,
                   "throwDecompress": r.random() < 0.15, "creates": []}
        zips[p]["creates"] = list(zips[p]["dirs"])
    if r.random() < 0.4:
        files.add(users + "_Imported/z0.zip")
        dirs.add(users + "_Imported")
    move_fails = [p for p in sorted(files) if p.endswith(".zip") and r.random() < 0.1]
    answers = [rng_pick(r, [0, 1, None]) for _ in range(8)]
    w = {"dirs": sorted(dirs), "files": sorted(files), "zips": zips, "moveFails": move_fails}
    im = Importer(w)
    im.popups_log = []
    im.import_new()
    # the host answers the popups in order; a click may spawn another popup
    ai = 0
    seen = 0
    while seen < len(im.popups):
        p = im.popups[seen]
        seen += 1
        im.popups_log.append(p)
        a = answers[ai % len(answers)]
        ai += 1
        if a is not None:
            im.click(p, a)
    return {"kind": "importing", "world": w, "answers": answers, "out": im.snapshot()}


# ======================================================================================================================================
# MissionEditorHistory
# ======================================================================================================================================
class HAction:
    def __init__(self, target, destroy_on_clear, state_id, destroy_targets, data):
        self.target, self.destroy_on_clear, self.state_id, self.destroy_targets, self.data = target, destroy_on_clear, state_id, list(destroy_targets), data


class History:
    def __init__(self, gen):
        self.gen = gen
        self.undo = []   # groups
        self.redo = []
        self.last_undo = -1
        self.last_redo = -1
        self.in_progress = False
        self.frame = 0
        self.state = EMPTY_GUID
        self.mission = "normal"    # normal | tutorial | none
        self.states = {}           # target -> counter (GetState)
        self.selected = []
        self.destroyed = []
        self.destroyed_targets = []
        self.callbacks = []

    def get_state(self, target):
        """IMEHistoryTarget.GetState(): a node with the number of GetState calls on that target"""
        self.states[target] = self.states.get(target, 0) + 1
        return "S{v=%d;}" % self.states[target]

    def push(self, stack, last_name, item):
        if getattr(self, last_name) != self.frame:
            stack.append([item])
            setattr(self, last_name, self.frame)
        else:
            if not stack:
                raise IndexError()
            stack[-1].append(item)

    def push_undo(self, target, destroy_on_clear, destroy_targets):
        if self.mission == "none":
            raise KeyError("NRE")
        if not self.in_progress and self.mission != "tutorial":
            data = self.get_state(target)     # the HistoryAction constructor reads the state of the target first
            self.push(self.undo, "last_undo", HAction(target, destroy_on_clear, self.state, destroy_targets, data))
            self.state = self.gen.next()
            self.clear_redo()

    def clear_redo(self):
        while self.redo:
            grp = self.redo.pop()
            for a in grp:
                if not a.destroy_on_clear:
                    continue
                if a.destroy_targets:
                    for h in a.destroy_targets:
                        self.destroyed.append(h)
                else:
                    self.destroyed_targets.append(a.target)

    def do(self, from_stack, to_stack, to_last, name, throws_at):
        if not from_stack:
            return
        self.in_progress = True
        try:
            grp = from_stack.pop()
            for i, a in enumerate(grp):
                copy_a = HAction(a.target, a.destroy_on_clear, a.state_id, a.destroy_targets, self.get_state(a.target))
                self.push(to_stack, to_last, copy_a)
                self.callbacks.append([a.target, a.data, name])
                if throws_at is not None and a.target == throws_at:
                    raise RuntimeError()
                self.selected.append(a.target)
                if self.mission == "none":
                    raise KeyError("NRE")
                self.state = a.state_id
        except (RuntimeError, KeyError, IndexError):
            self.clear()
        self.in_progress = False

    def clear(self):
        self.undo = []
        self.last_undo = -1
        self.clear_redo()
        # RedoActions.Clear() is not called by Clear(): ClearRedoActions only pops the groups

    def view(self):
        def grp(stack):
            return [[[a.target, a.data, a.state_id, a.destroy_on_clear, a.destroy_targets] for a in g] for g in stack]
        return {"undo": copy.deepcopy(grp(self.undo)), "redo": copy.deepcopy(grp(self.redo)), "inProgress": self.in_progress, "hasHistory": len(self.undo) > 0, "state": self.state,
                "selected": list(self.selected), "destroyed": list(self.destroyed), "destroyedTargets": list(self.destroyed_targets),
                "callbacks": copy.deepcopy(self.callbacks)}


def gen_history(r):
    gen = GuidGen(3000)
    h = History(gen)
    ops = [{"op": "init", "guid0": 3000}]
    for k in range(r.randrange(10, 26)):
        t = rng_pick(r, ["push", "push", "push", "undo", "undo", "redo", "frame", "frame", "clear", "mission", "throw"])
        if t == "push":
            tg = rng_pick(r, [1, 2, 3])
            d = r.random() < 0.5
            dt = [rng_pick(r, [10, 11, 12]) for _ in range(r.randrange(0, 3))] if r.random() < 0.5 else []
            ops.append({"op": "push", "target": tg, "destroyOnClear": d, "destroyTargets": dt})
            try:
                h.push_undo(tg, d, dt)
                ops[-1]["out"] = h.view()
            except KeyError:
                ops[-1]["out"] = "throws"
            except IndexError:
                ops[-1]["out"] = "throwsIndex"
        elif t == "undo":
            ops.append({"op": "undo"})
            h.do(h.undo, h.redo, "last_redo", "undo", h.throw_target if hasattr(h, "throw_target") else None)
            ops[-1]["out"] = h.view()
        elif t == "redo":
            ops.append({"op": "redo"})
            h.do(h.redo, h.undo, "last_undo", "redo", h.throw_target if hasattr(h, "throw_target") else None)
            ops[-1]["out"] = h.view()
        elif t == "frame":
            h.frame += 1
            ops.append({"op": "frame", "value": h.frame})
        elif t == "clear":
            ops.append({"op": "clear"})
            h.clear()
            ops[-1]["out"] = h.view()
        elif t == "mission":
            h.mission = rng_pick(r, ["normal", "tutorial", "normal", "none"])
            ops.append({"op": "mission", "value": h.mission})
        elif t == "throw":
            h.throw_target = rng_pick(r, [None, 1, 2, 3])
            ops.append({"op": "throwOn", "target": h.throw_target})
    return {"kind": "history", "ops": ops}


def generate(r):
    cases = []
    cases += [gen_scoreinfo(r) for _ in range(60)]
    cases += [gen_localizer(r) for _ in range(40)]
    cases += [gen_importing(r) for _ in range(120)]
    cases += [gen_history(r) for _ in range(80)]
    return cases
