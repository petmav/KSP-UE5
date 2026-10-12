"""WP-54c reference, part 3: the craft-file bookkeeping of Mission.cs (RebuildCraftFileList, ProcessCraftRenames, AddMappedPartVesselName, AddMappedCraft,
ClearMappedVessels, the Get*Craft* lookups), MissionCraft.cs, the vessel / part event handlers that keep mappedVessels current and
ProcessMappedVesselNodeMap / RemapVesselId (Mission.cs:1217-1348, 1350-1640, 1964-2175). Written from the C#; the file system is an in-memory dictionary on
both sides (labelled synthetic: the stock .craft files of the mirror are 0-byte files). No kspcore code."""
import copy

from ref_c_common import *  # noqa: F401,F403
from ref_c_common import (Cfg, CfgR, fmt, guid_of, rng_pick, MappedVessel, mv_index_part, mv_index_mapped, mv_index_current, mv_convert, mv_exists, mv_find, TICK)
from ref_partactions import VPair

FACILITIES = ["VAB", "SPH"]
CRAFT_NAMES = ["alpha", "bravo", "charlie", "delta", "echo", "foxtrot"]
SHIP_NAMES = ["Rocket A", "Rocket B", "Plane", "Rover"]
VESSEL_NAMES = ["Booster", "Lander", "Probe", "Rescue"]
UID0 = 9000


class Uid:
    """FlightGlobals.GetUniquepersistentId (the test pins a counter on both sides)"""

    def __init__(self, start):
        self.n = start

    def next(self):
        v = self.n
        self.n += 1
        return v


class FS:
    def __init__(self, files, dirs):
        self.files = {k: copy.deepcopy(v) for k, v in files.items()}
        self.dirs = set(dirs)

    def exists(self, path):
        return path in self.files

    def load(self, path):
        return copy.deepcopy(self.files[path]) if path in self.files else None

    def save(self, cfg, path):
        self.files[path] = copy.deepcopy(cfg)

    def list_craft(self, folder):
        names = sorted(p[len(folder):] for p in self.files if p.startswith(folder) and "/" not in p[len(folder):] and p.endswith(".craft"))
        return names


def get_value(c, key):
    return c.get(key)


class Craft:
    """MissionCraft.cs"""

    def __init__(self, fs, uid, folder, file):
        self.fs, self.uid = fs, uid
        self.craftFile, self.craftFolder = file, folder
        self.facility = ""
        self.mappedItem = False
        self.mappedVesselName = ""
        self.craftPersistentId = 0
        self.rootPartPersistentId = 0
        self.cache = None
        last = folder.rstrip("/").split("/")[-1]
        if last == "VAB":
            self.facility = "VAB"
        elif last == "SPH":
            self.facility = "SPH"
        self.check_ids()

    def node(self):
        if self.cache is None:
            base = self.craftFolder.rstrip("/")
            if self.fs.exists(base + "/" + self.craftFile):
                self.cache = self.fs.load(base + "/" + self.craftFile)
            elif self.fs.exists(base + "/VAB/" + self.craftFile):
                self.cache = self.fs.load(base + "/VAB/" + self.craftFile)
                self.facility = "VAB"
            elif self.fs.exists(base + "/SPH/" + self.craftFile):
                self.cache = self.fs.load(base + "/SPH/" + self.craftFile)
                self.facility = "SPH"
        return self.cache

    def vessel_name(self):
        n = self.node()
        if n is not None:
            v = n.get("ship")
            if v is not None:
                return v
        return ""

    def part_id(self, pnode):
        if pnode is None:
            return 0
        v = pnode.get("persistentId")
        if v is not None:
            return int(v)
        u = self.uid.next()
        pnode.values.append(("persistentId", str(u)))
        return u

    def check_ids(self):
        if self.node() is None:
            return
        flag = False
        c = copy.deepcopy(self.cache)
        if c.get("persistentId") is None:
            c.values.append(("persistentId", str(self.uid.next())))
            flag = True
        self.craftPersistentId = int(c.get("persistentId"))
        parts = [n for n in c.nodes if n.name == "PART"]
        for i, p in enumerate(parts):
            if p.get("persistentId") is None:
                p.values.append(("persistentId", str(self.uid.next())))
                flag = True
            if i == 0:
                self.rootPartPersistentId = int(p.get("persistentId"))
        if flag:
            self.fs.save(c, self.craftFolder + self.craftFile)
            self.cache = None

    def clear(self):
        self.cache = None

    def parts(self):
        n = self.node()
        return [x for x in n.nodes if x.name == "PART"] if n is not None else []


class CraftMission:
    def __init__(self, fs, uid, vessels, mapped):
        self.fs, self.uid = fs, uid
        self.vessels = vessels                   # [{"pid","craft","name"}]
        self.mapped = [m.copy() for m in mapped]
        self.keys = []
        self.craftList = []                      # [(key, Craft)]
        self.refresh = 0

    # ---- DictionaryValueList<string, MissionCraft> ---------------------------------------------------------------------------------
    def cl_remove(self, key):
        self.craftList = [(k, c) for k, c in self.craftList if k != key]

    def cl_add(self, key, craft):
        self.cl_remove(key)
        self.craftList.append((key, craft))

    def cl_get(self, key):
        for k, c in self.craftList:
            if k == key:
                return c
        return None

    def new_mapped(self, part, orig, cur, pvn, craft, sit):
        return MappedVessel(part, orig, cur, pvn, craft, sit, self.uid.next())

    # ---- Mission.cs:@RebuildCraftFileList ------------------------------------------------------------------------------------------
    def rebuild(self):
        for _, c in self.craftList:
            c.clear()
        self.craftList = []
        self.keys = []
        self.process_folder("SHIPS/VAB/")
        self.process_folder("SHIPS/SPH/")
        # ClearMappedVessels
        n = len(self.mapped)
        while n > 0:
            n -= 1
            m = self.mapped[n]
            if not mv_exists(self.keys, m.cur, m.part, m.pvn, m.craft):
                del self.mapped[n]

    def process_folder(self, folder):
        if folder in self.fs.dirs:
            for name in self.fs.list_craft(folder):
                craft = Craft(self.fs, self.uid, folder, name)
                self.process_renames(name, craft)

    def sit_by_craft(self, craft_file):
        for v in self.vessels:
            if v["craft"] == craft_file:
                return v
        return None

    def process_renames(self, craft_name, craft):
        node = craft.node()
        sit = self.sit_by_craft(craft.craftFile)
        if sit is not None:
            value = sit["pid"]
        else:
            if craft.node() is None:
                raise KeyError("NRE")
            value = int(craft.node().get("persistentId") or 0)
        if node is None or value == 0:
            return
        if node.get("ship") is None:
            return
        parts = [n for n in node.nodes if n.name == "PART"]
        for i, p in enumerate(parts):
            num = craft.part_id(p)
            vn = next((n for n in p.nodes if n.name == "VESSELNAMING"), None)
            if vn is not None:
                name = vn.get("name") or ""
                if name != "":
                    found = mv_find(self.mapped, value, num, name, craft.craftFile)
                    if found is None:
                        found = self.add_mapped_part(num, value, name, craft, True, sit)
                    else:
                        found = found.copy()
                    if not mv_exists(self.keys, value, num, name, craft.craftFile):
                        self.keys.append(found)
                    if i == 0:
                        self.add_mapped_craft(craft_name, craft)
            elif i == 0:
                self.add_mapped_part(num, value, craft_name, craft, False, sit)

    def add_mapped_part(self, part, vsl, craft_name, craft, mapped_item, sit):
        text = craft_name if mapped_item else craft.vessel_name()
        mc = Craft(self.fs, self.uid, craft.craftFolder, craft.craftFile)
        mc.mappedVesselName = text
        mc.mappedItem = mapped_item
        if not mapped_item:
            self.cl_remove(craft_name)
        mm = self.new_mapped(part, sit["pid"] if sit is not None else vsl, vsl, text, craft.craftFile, fmt(sit["name"]) if sit is not None else "")
        self.keys.append(mm.copy())
        if not mapped_item:
            self.cl_add(craft_name, mc)
        if not mv_exists(self.mapped, vsl, part, text, craft.craftFile):
            self.mapped.append(mm.copy())
        return mm

    def add_mapped_craft(self, craft_name, craft):
        vname = craft.vessel_name()
        mc = Craft(self.fs, self.uid, craft.craftFolder, craft.craftFile)
        mc.mappedVesselName = vname
        mc.mappedItem = False
        self.cl_remove(craft_name)
        self.cl_add(craft_name, mc)

    # ---- lookups ------------------------------------------------------------------------------------------------------------------
    def craft_by_vessel(self, pid):
        pid = mv_convert(self.mapped, pid)
        for v in self.vessels:
            if v["pid"] == pid:
                return self.cl_get(v["craft"])
        return None

    def sit_index_by_vessel(self, pid):
        pid = mv_convert(self.mapped, pid)
        for i, v in enumerate(self.vessels):
            if v["pid"] == pid:
                return i
        return -1

    def sit_index_by_part(self, pid):
        for i, v in enumerate(self.vessels):
            c = self.cl_get(v["craft"])
            if c is None:
                continue
            if c.node() is None:
                raise KeyError("NRE")
            for p in c.parts():
                if c.part_id(p) == pid:
                    return i
        return -1

    def craft_index_by_part(self, pid):
        for i, (k, c) in enumerate(self.craftList):
            if c.node() is None:
                continue
            for p in c.parts():
                if c.part_id(p) == pid:
                    return i
        return 0

    def craft_by_name(self, name):
        for k, c in self.craftList:
            if c.vessel_name() == name:
                return c
        return None

    # ---- event handlers -----------------------------------------------------------------------------------------------------------
    def on_part_naming(self, part_pid, name, vessel_pid):
        flag = False
        for m in self.mapped:
            if m.part == part_pid:
                m.pvn = name
                if vessel_pid is not None:
                    m.cur = vessel_pid
                flag = True
        if flag:
            self.refresh += 1

    def on_rename(self, vessel_pid, to):
        flag = False
        for v in self.vessels:
            if v["pid"] == vessel_pid:
                v["name"] = to
                flag = True
        for m in self.mapped:
            if m.cur == vessel_pid:
                m.sit = to
                flag = True
        if flag:
            self.refresh += 1

    def on_decouple(self, old, new, new_parts):
        flag = False
        for m in self.mapped:
            if m.cur == old and m.part in new_parts:
                m.cur = new
                flag = True
        if flag:
            self.refresh += 1

    def on_vessel_id(self, old, new):
        flag = False
        for m in self.mapped:
            if m.cur == old:
                m.cur = new
                flag = True
        if flag:
            self.refresh += 1

    def on_part_id(self, vid, old, new):
        flag = False
        for m in self.mapped:
            if m.cur == vid and m.part == old:
                m.part = new
                flag = True
        if flag:
            self.refresh += 1

    def snapshot(self, keys=False):
        out = {"craft": [[k, c.craftFile, c.craftFolder, c.facility, c.mappedItem, c.mappedVesselName, c.craftPersistentId, c.rootPartPersistentId] for k, c in self.craftList],
                "mapped": [m.dump() for m in self.mapped], "fs": {k: v.dump() for k, v in sorted(self.fs.files.items())},
                "uid": self.uid.n, "refresh": self.refresh,
                "vessels": [[v["pid"], v["craft"], v["name"]] for v in self.vessels]}
        if keys:     # mappedVesselKeys is private and rebuilt by every RebuildCraftFileList; the C# keeps references shared with mappedVessels, the C++ copies
            out["keys"] = [m.dump() for m in self.keys]
        return out


# ---- ProcessMappedVesselNodeMap ---------------------------------------------------------------------------------------------------------
def upgrade_id(mapped, cur):
    k = mv_index_current(mapped, cur)
    return mapped[k].mapped if k != -1 else cur


def process_node_map(mapped, ids, mapped_id=0, new_id=0):
    """ids: list of {"kind", "key", "value"} the module fields; returns the new values (kinds: every kind below is remapped alike)"""
    flag = new_id != 0 and mapped_id != 0
    out = []
    for e in ids:
        v = e["value"]
        if v != 0:
            if flag:
                if v == mapped_id:
                    v = new_id
            else:
                v = upgrade_id(mapped, v)
        out.append(v)
    return out


# ---- scenario generation ------------------------------------------------------------------------------------------------------------
def gen_craft_cfg(r, ship, parts, ids=None):
    c = Cfg("")
    c.values.append(("version", "1.12.5"))
    c.values.append(("ship", ship))
    if r.random() < 0.5:
        c.values.append(("persistentId", str(r.randrange(100, 9000))))
    for i in range(parts):
        p = Cfg("PART")
        p.values.append(("part", "mk1pod_%d" % i))
        if r.random() < 0.5:
            p.values.append(("persistentId", str(r.randrange(10000, 60000))))
        if r.random() < 0.6:
            vn = Cfg("VESSELNAMING")
            vn.values.append(("name", rng_pick(r, VESSEL_NAMES)))
            p.nodes.append(vn)
        c.nodes.append(p)
    return c


def gen_case(r):
    files = {}
    dirs = set()
    for fac in FACILITIES:
        if r.random() < 0.85:
            dirs.add("SHIPS/%s/" % fac)
            for name in r.sample(CRAFT_NAMES, r.randrange(0, 4)):
                files["SHIPS/%s/%s.craft" % (fac, name)] = gen_craft_cfg(r, rng_pick(r, SHIP_NAMES), r.randrange(0, 4))
    craft_files = sorted(p.split("/")[-1] for p in files)
    vessels = []
    for i in range(r.randrange(0, 4)):
        cf = rng_pick(r, craft_files + [""] + ["missing.craft"]) if craft_files else ""
        vessels.append({"pid": 100 + 7 * i, "craft": cf, "name": rng_pick(r, ["Alpha", "Bravo", "#autoLOC_8100086"])})
    mapped = []
    for i in range(r.randrange(0, 3)):
        mapped.append(MappedVessel(r.randrange(10000, 60000), 100 + 7 * i, rng_pick(r, [100, 107, 114, 999]), rng_pick(r, VESSEL_NAMES), rng_pick(r, craft_files + ["x.craft"]) if craft_files else "x.craft",
                                   rng_pick(r, ["", "Alpha"]), 700 + i))
    uid = Uid(UID0)
    fs = FS(files, dirs)
    m = CraftMission(fs, uid, copy.deepcopy(vessels), mapped)
    spec = {"files": {k: v.to_json() for k, v in files.items()}, "dirs": sorted(dirs), "vessels": vessels, "mapped": [mm.to_json() for mm in mapped], "uid": UID0}
    ops = []
    try:
        m.rebuild()
        ops.append({"op": "rebuild", "out": m.snapshot(True)})
    except KeyError:
        ops.append({"op": "rebuild", "out": "throws"})
        return {"kind": "craft", "spec": spec, "ops": ops}
    # lookups
    for pid in [100, 107, 114, 700, 701, 999, 5]:
        c = m.craft_by_vessel(pid)
        ops.append({"op": "craftByVessel", "pid": pid, "out": None if c is None else c.craftFile})
    for name in craft_files + ["nothing.craft"]:
        c = m.cl_get(name)
        ops.append({"op": "craftByFile", "name": name, "out": None if c is None else [c.craftFile, c.craftFolder, c.mappedVesselName, c.mappedItem]})
        s = m.sit_by_craft(name)
        ops.append({"op": "sitByCraft", "name": name, "out": None if s is None else s["pid"]})
    for name in SHIP_NAMES:
        c = m.craft_by_name(name)
        ops.append({"op": "craftByName", "name": name, "out": None if c is None else c.craftFile})
    for pid in [100, 107, 114, 700, 701]:
        ops.append({"op": "sitIndexByVessel", "pid": pid, "out": m.sit_index_by_vessel(pid)})
    part_ids = [int(p.get("persistentId")) for fn, c in m.fs.files.items() for p in c.nodes if p.name == "PART"] + [1]
    for pid in part_ids[:6]:
        try:
            ops.append({"op": "sitIndexByPart", "pid": pid, "out": m.sit_index_by_part(pid)})
        except KeyError:
            ops.append({"op": "sitIndexByPart", "pid": pid, "out": "throws"})
        ops.append({"op": "craftIndexByPart", "pid": pid, "out": m.craft_index_by_part(pid)})
    # events
    vids = [100, 107, 114, 999]
    for k in range(6):
        t = rng_pick(r, ["naming", "rename", "decouple", "vesselId", "partId", "docking", "undocking"])
        pids = [m_.part for m_ in m.mapped] + [4242]
        if t == "naming":
            pp, nm, vp = rng_pick(r, pids), rng_pick(r, VESSEL_NAMES), rng_pick(r, vids + [None])
            m.on_part_naming(pp, nm, vp)
            ops.append({"op": "evNaming", "part": pp, "name": nm, "vessel": vp, "out": m.snapshot()})
        elif t == "rename":
            vp, to = rng_pick(r, vids), rng_pick(r, VESSEL_NAMES)
            m.on_rename(vp, to)
            ops.append({"op": "evRename", "vessel": vp, "to": to, "out": m.snapshot()})
        elif t in ("decouple", "undocking"):
            old, new = rng_pick(r, vids), rng_pick(r, [200, 201, 114])
            np_ = [x for x in pids if r.random() < 0.5]
            m.on_decouple(old, new, np_)
            ops.append({"op": "evDecouple" if t == "decouple" else "evUndocking", "old": old, "new": new, "parts": np_, "out": m.snapshot()})
        elif t in ("vesselId", "docking"):
            old, new = rng_pick(r, vids), rng_pick(r, [300, 301])
            m.on_vessel_id(old, new)
            ops.append({"op": "evVesselId" if t == "vesselId" else "evDocking", "old": old, "new": new, "out": m.snapshot()})
        else:
            v, old = rng_pick(r, vids), rng_pick(r, pids)
            new = old + 1
            m.on_part_id(v, old, new)
            ops.append({"op": "evPartId", "vessel": v, "old": old, "new": new, "out": m.snapshot()})
    # a second rebuild after adding a craft file
    newcfg = gen_craft_cfg(r, rng_pick(r, SHIP_NAMES), r.randrange(1, 3))
    m.fs.files["SHIPS/VAB/zulu.craft"] = newcfg
    m.fs.dirs.add("SHIPS/VAB/")
    ops.append({"op": "addFile", "path": "SHIPS/VAB/zulu.craft", "cfg": newcfg.to_json()})
    try:
        m.rebuild()
        ops.append({"op": "rebuild", "out": m.snapshot(True)})
    except KeyError:
        ops.append({"op": "rebuild", "out": "throws"})
    return {"kind": "craft", "spec": spec, "ops": ops}


# ---- ProcessMappedVesselNodeMap scenarios -------------------------------------------------------------------------------------------------
MOD_KINDS = ["test:TestVesselMass", "test:TestVesselAltitude", "test:TestDistance:to", "test:TestDistance:from", "test:TestPartDocking:one", "test:TestPartDocking:two",
             "test:TestCrewAssignment", "action:ActionPartExplode", "action:ActionPartFailure", "action:ActionPartRepair", "action:ActionPartResourceDrain"]


def gen_nodemap(r):
    mapped = []
    for i in range(r.randrange(1, 4)):
        mapped.append(MappedVessel(5000 + i, 100 + 7 * i, rng_pick(r, [100, 107, 114, 999, 150]), "Booster", "c%d.craft" % i, "", 700 + i))
    mods = []
    for k in range(r.randrange(3, 9)):
        kind = rng_pick(r, MOD_KINDS)
        mods.append({"kind": kind, "value": rng_pick(r, [0, 100, 107, 114, 999, 150, 700, 701, 702, 12345])})
    ops = []
    # UpgradeMappedVesselId: every id that equals a currentVesselPersistentId becomes the mappedVesselPersistentId
    ops.append({"op": "process", "mapped": 0, "new": 0, "out": process_node_map(mapped, mods)})
    vals = process_node_map(mapped, mods)
    # RemapVesselId(current): ProcessMappedVesselNodeMap(mappedId, currentId) and the mapped entry is removed
    mods2 = [dict(x, value=v) for x, v in zip(mods, vals)]
    cur = rng_pick(r, [mm.cur for mm in mapped] + [12345])
    k = mv_index_current(mapped, cur)
    mapped_after = [mm.copy() for mm in mapped]
    if k != -1:
        mid, cid = mapped[k].mapped, mapped[k].cur
        vals2 = process_node_map(mapped, mods2, mid, cid)
        del mapped_after[k]
    else:
        vals2 = [x["value"] for x in mods2]
    ops.append({"op": "remap", "cur": cur, "out": vals2, "mapped": [mm.dump() for mm in mapped_after]})
    return {"kind": "nodemap", "mapped": [mm.to_json() for mm in mapped], "mods": mods, "ops": ops}


def generate(r):
    cases = [gen_case(r) for _ in range(150)]
    cases += [gen_nodemap(r) for _ in range(60)]
    return cases
