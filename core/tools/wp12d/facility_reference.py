"""WP-12d independent reference: which KSC scenery objects are in the physics scene for a facility-level state and a set of destroyed
buildings. Written from the C# with no kspcore code:
  * the level of a facility: ScenarioUpgradeableFacilities.OnLoad -> RegisterUpgradeable -> UpgradeableFacility.Load -> setNormLevel
    (UpgradeableFacility.cs:130: FloorToInt(Clamp01(lvl) * (MaxLevel + 1)), clamped); an id the save does not list: Reset -> MaxLevel
    (UpgradeableFacility.cs:80); a run-time UpgradeableFacility.SetLevel(n) (:85);
  * the level of a slave: UpgradeableSlave.UpdateLevel (UpgradeableSlave.cs:70): the neighbours' GetFacilityLevel (ProtoUpgradeable.GetLevel
    = facilityLevel / MaxLevel of the registered facility, ScenarioUpgradeableFacilities.cs:71) summed last to first in float, divided by
    the count when > 1, FloorToInt(avg * level count), clamped;
  * presence: the objects of a level group's other levels (and their subtrees) are not in the scene (UpgradeLevel.Spawn / Despawn);
  * Unity activation: an object is in the scene iff it and every ancestor are activeSelf; PQSCity.OnUpdateFinished with the target at the
    city re-activates the city's direct children (transformsActive, PQSCity.cs:247) and every LODRange in range (:281);
  * a destroyed building: CollapsibleObject.SetDestroyed(true) (DestructibleBuilding.cs:163): collapse objects inactive, replacements active;
    an intact building is Loaded or Reset at Start (building_states): SetDestroyed(false), every collapse object active.
Floats under the Mono rule (tools/wp12/pqs_ref_math.py f32). Input: core/data/scenery/scenery_colliders.json (the KSC city).
The slave neighbour ids are a FIXTURE (not in data/scenery: UpgradeableSlave.neighbourIDs was not extracted; the ids below follow the
group names, e.g. Grounds/VAB_RnD1 between the VehicleAssemblyBuilding and ResearchAndDevelopment).

  python -I tools/wp12d/facility_reference.py   -> tests/data/wp12d_facility_reference.json
"""
import hashlib
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(CORE, "tools", "wp12"))
import pqs_ref_math as M  # noqa: E402

f32 = M.f32
DATA = os.path.join(CORE, "data", "scenery", "scenery_colliders.json")
OUT = os.path.join(CORE, "tests", "data", "wp12d_facility_reference.json")

ABBR = {"AC": "SpaceCenter/AstronautComplex", "RnD": "SpaceCenter/ResearchAndDevelopment", "SPH": "SpaceCenter/SpaceplaneHangar",
        "VAB": "SpaceCenter/VehicleAssemblyBuilding", "TS": "SpaceCenter/TrackingStation", "MC": "SpaceCenter/MissionControl"}


def slave_fixture(groups):
    out = {}
    for g in groups:
        if g.startswith("SpaceCenter/Grounds/"):
            parts = g.split("/")[-1].split("_")
            out[g] = [ABBR[p.rstrip("0123456789")] for p in parts]
    return out


def floor_to_int(f):
    return int(math.floor(f))


def facility_level(lvl, levels):
    """UpgradeableFacility.setNormLevel (:130); None: not in the save -> Reset -> MaxLevel"""
    max_level = levels - 1
    if lvl is None:
        return max_level
    lvl = min(max(lvl, 0.0), 1.0)
    return min(max(floor_to_int(f32(lvl * float(max_level + 1))), 0), max_level)


def slave_level(neigh, facility_levels, levels_of, levels):
    avg = 0.0
    for nid in reversed(neigh):
        fl, mx = facility_levels[nid], levels_of[nid] - 1
        avg = f32(avg + f32(float(fl) / float(mx)))   # GetNormLevel (float division)
    if len(neigh) > 1:
        avg = f32(avg / float(len(neigh)))
    return min(max(floor_to_int(f32(avg * float(levels))), 0), levels - 1)


def group_level(city, i):
    objs = city["objects"]
    while i >= 0 and "levelGroup" not in objs[i]:
        i = objs[i]["parent"]
    return (objs[i]["levelGroup"], objs[i]["level"]) if i >= 0 else None


def building_states(city, saved_destroyed):
    """the CollapsibleObject state of every destructible record when its level is spawned: DestructibleBuilding.Start
    (DestructibleBuilding.cs:260) registers the id with ScenarioDestructibles; a known id with a saved node is Loaded
    (DestructibleBuilding.Load -> CollapsibleObject.SetDestroyed(!intact), :351), every other case returns false and Start calls Reset
    (:495 -> CollapsibleObject.Reset -> SetDestroyed(false)). Either way every collapse object ends active (intact) or inactive
    (destroyed) and the replacements the other way, whatever the prefab's activeSelf. Returns {record index: intact}"""
    return {r: d["id"] not in saved_destroyed or not d["id"] for r, d in enumerate(city["destructibles"])}


def scene(city, levels, saved_destroyed):
    objs = city["objects"]
    n = len(objs)
    group_of = [group_level(city, i) for i in range(n)]
    present = [g is None or levels[g[0]] == g[1] for g in group_of]
    active = [o["active"] for o in objs]
    for c in city["children"]:
        active[c] = True
    for l in city["lod"]:
        for o in l["objects"]:
            active[o] = True
    states = building_states(city, saved_destroyed)
    for r, d in enumerate(city["destructibles"]):
        if not present[d["object"]]:
            continue
        intact = states[r]
        for c in d["collapsibles"]:
            active[c["collapse"]] = intact
            if c["replacement"] >= 0:
                active[c["replacement"]] = not intact
    placed = []
    for i in range(n):
        k, ok = i, present[i]
        while ok and k >= 0:
            ok = active[k]
            k = objs[k]["parent"]
        if ok:
            placed.append(i)
    return placed


def main():
    doc = json.load(open(DATA, encoding="utf-8"))
    city = [c for c in doc["cities"] if c["name"] == "Kerbin/KSC"][0]
    levels_of = {}
    for o in city["objects"]:
        if "levelGroup" in o:
            levels_of[o["levelGroup"]] = max(levels_of.get(o["levelGroup"], 0), o["level"] + 1)
    slaves = slave_fixture(levels_of)
    facilities = sorted(g for g in levels_of if g not in slaves)
    mixed = {"SpaceCenter/Administration": 0.5, "SpaceCenter/AstronautComplex": 1.0, "SpaceCenter/LaunchPad": 0.5,
             "SpaceCenter/MissionControl": 0.0, "SpaceCenter/ResearchAndDevelopment": 1.0, "SpaceCenter/Runway": 0.5,
             "SpaceCenter/SpaceplaneHangar": 0.0, "SpaceCenter/TrackingStation": 1.0, "SpaceCenter/VehicleAssemblyBuilding": 0.5}
    scenarios = [
        {"name": "fresh_career", "lvl": {g: 0.0 for g in facilities if g in mixed}, "upgrade": {}, "destroyed": []},
        {"name": "mixed", "lvl": mixed, "upgrade": {}, "destroyed": []},
        {"name": "mixed_destroyed", "lvl": mixed, "upgrade": {}, "destroyed": []},
        {"name": "mixed_upgrade", "lvl": mixed, "upgrade": {"SpaceCenter/LaunchPad": 2, "SpaceCenter/VehicleAssemblyBuilding": 2}, "destroyed": []},
    ]
    out = []
    for sc in scenarios:
        fl = {g: facility_level(sc["lvl"].get(g), levels_of[g]) for g in facilities}
        fl.update(sc["upgrade"])
        lv = dict(fl)
        for g, neigh in slaves.items():
            lv[g] = slave_level(neigh, fl, levels_of, levels_of[g])
        if sc["name"] == "mixed_destroyed":   # three buildings of the shown levels
            ids = []
            for d in city["destructibles"]:
                o = d["object"]
                k = o
                while k >= 0 and "levelGroup" not in city["objects"][k]:
                    k = city["objects"][k]["parent"]
                if k >= 0 and lv[city["objects"][k]["levelGroup"]] == city["objects"][k]["level"] and d["id"] not in ids:
                    ids.append(d["id"])
            sc["destroyed"] = [ids[0], ids[5], ids[9]]
        placed = scene(city, lv, set(sc["destroyed"]))
        out.append({"name": sc["name"], "lvl": {g: M.fbits(f32(v)) for g, v in sc["lvl"].items()}, "upgrade": sc["upgrade"],
                    "destroyed": sc["destroyed"], "levels": lv, "placed": placed, "placedCount": len(placed)})
    res = {"schema": "kspcore-wp12d-facilities/1",
           "provenance": {"tool": "core/tools/wp12d/facility_reference.py",
                          "data": "core/data/scenery/scenery_colliders.json (sha256 %s)" % hashlib.sha256(open(DATA, "rb").read()).hexdigest(),
                          "fixture": "UpgradeableSlave.neighbourIDs from the group names (not extracted)"},
           "slaves": slaves, "scenarios": out}
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(res, f, sort_keys=True, separators=(",", ":"))
        f.write("\n")
    for s in out:
        print(s["name"], s["placedCount"], "placed;", {g: l for g, l in s["levels"].items()})


if __name__ == "__main__":
    main()
