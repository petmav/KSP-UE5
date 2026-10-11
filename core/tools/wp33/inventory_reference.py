#!/usr/bin/env python3
"""WP-33: independent reference for the inventory / EVA-construction numbers -- test data generator.

Re-implements from the decompiled C# (no kspcore code, no kspcore output) the arithmetic and bookkeeping of
  ModuleInventoryPart: UpdateCapacityValues (ModuleInventoryPart.cs:1429), GetPartMass (:1571), GetModuleMass (:1928), GetModuleCost (:1966),
    HasCapacity (:1576), PreviewLimits (:1508), the over-limit flags (UpdateMassVolumeDisplay :1461), StoreCargoPartAtSlot (:762),
    ClearPartAtSlot (:847), UpdateStackAmountAtSlot (:429), RemoveNPartsFromInventory (:1881), FirstEmptySlot (:1701),
    TotalAmountOfPartStored (:1862), the legacy `inventory` string (:1130),
  ShipConstruction.GetPartCosts (ShipConstruction.cs:1202),
  EVA construction: Part.IsUnderConstructionWeightLimit (Part.cs:4060), EVAConstructionModeEditor.UpdateAssistingKerbals (:1265, the limit
    sum), EVAConstructionModeController.UpdateInfoLabels (:604, the "{0:F2}t" mass text).
Inputs are game data only: the PART / RESOURCE_DEFINITION cfgs of core/testdata/GameData-mirror (Squad parts that carry ModuleCargoPart with
packedVolume > 0, the five stock inventory containers) and Physics.cfg (constructionWeightLimit). The part selection and slot permutation
come from a fixed LCG. Float32 stores use numpy.float32 per the Mono float rule (a float expression is evaluated in double and rounded only
where the C# stores / passes / returns it as a float; an explicit (float) cast rounds).

Usage: python -I tools/wp33/inventory_reference.py > tests/data/wp33_reference.json
"""
import json
import math
import os
import re
import sys
from decimal import Decimal, ROUND_HALF_UP

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.normpath(os.path.join(HERE, "..", ".."))
MIRROR = os.path.join(CORE, "testdata", "GameData-mirror")
PARTS = os.path.join(MIRROR, "GameData", "Squad", "Parts")
RES = os.path.join(MIRROR, "GameData", "Squad", "Resources", "ResourcesGeneric.cfg")
PHYSICS = os.path.join(MIRROR, "Physics.cfg")


def f32(x):
    return float(np.float32(x))


def bits(x):
    return int(np.float32(x).view(np.uint32))


def parse_cfg(text):
    """Minimal KSP cfg tree: [(kind, name_or_key, value_or_children)]; // comments anywhere; the first token of a node line is its name."""
    root = []
    stack = [root]
    pending = None
    for raw in text.lstrip("﻿").splitlines():
        line = raw.split("//")[0].strip()
        if not line:
            continue
        if line == "{":
            node = {"name": pending, "values": [], "nodes": []}
            stack[-1].append(node) if isinstance(stack[-1], list) else stack[-1]["nodes"].append(node)
            stack.append(node)
            pending = None
            continue
        if line == "}":
            stack.pop()
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            if isinstance(stack[-1], dict):
                stack[-1]["values"].append((k.strip(), v.strip()))
            continue
        if line.endswith("{"):
            node = {"name": line[:-1].strip(), "values": [], "nodes": []}
            stack[-1].append(node) if isinstance(stack[-1], list) else stack[-1]["nodes"].append(node)
            stack.append(node)
            continue
        pending = line
    return root


def val(node, key, default=None):
    for k, v in node["values"]:
        if k == key:
            return v
    return default


def load_resources():
    out = {}
    for n in parse_cfg(open(RES, encoding="utf-8").read()):
        if n["name"] == "RESOURCE_DEFINITION":
            out[val(n, "name")] = {"density": f32(float(val(n, "density", "0"))), "unitCost": f32(float(val(n, "unitCost", "0")))}
    return out


def load_parts(resdefs):
    parts = {}
    inventories = {}
    for dp, _, fns in os.walk(PARTS):
        if "zDeprecated" in dp:
            continue
        for fn in sorted(fns):
            if not fn.endswith(".cfg"):
                continue
            tree = parse_cfg(open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read())
            for n in tree:
                if n["name"] != "PART":
                    continue
                name = val(n, "name").replace("_", ".")  # PartLoader: part names use '.' for '_' (PartLoader.cs ParsePart)
                mods = {m_name: m for m in n["nodes"] if m["name"] == "MODULE" for m_name in [val(m, "name")]}
                mass = f32(float(val(n, "mass", "0")))
                cost = f32(float(val(n, "cost", "0")))
                resources = []
                for r in n["nodes"]:
                    if r["name"] == "RESOURCE":
                        rn = val(r, "name")
                        resources.append({"name": rn, "amount": float(val(r, "amount", "0")), "maxAmount": float(val(r, "maxAmount", "0")),
                                          "density": resdefs[rn]["density"], "unitCost": resdefs[rn]["unitCost"]})
                if "ModuleInventoryPart" in mods:
                    m = mods["ModuleInventoryPart"]
                    host = {"name": name, "mass": mass, "resources": resources}
                    inventories[name] = {"slots": int(val(m, "InventorySlots", "9")), "volumeLimit": f32(float(val(m, "packedVolumeLimit", "0"))),
                                         "massLimit": f32(float(val(m, "massLimit", "0"))), "hostMass_bits": bits(part_mass(host))}
                elif "ModuleCargoPart" in mods:
                    m = mods["ModuleCargoPart"]
                    pv = f32(float(val(m, "packedVolume", "0")))
                    sq = int(val(m, "stackableQuantity", "1"))
                    if resources and sq > 1:  # ModuleCargoPart.OnLoad (:96)
                        sq = 1
                    parts[name] = {"name": name, "mass": mass, "cost": cost, "packedVolume": pv, "stackable": sq, "resources": resources}
    return parts, inventories


def resource_mass(p):
    # Part.GetResourceMass (Part.cs:6547): double accumulation, reverse index order, (float) at the return
    num = 0.0
    for r in reversed(p["resources"]):
        num += r["amount"] * r["density"]
    return f32(num)


def part_mass(p):
    # ModuleInventoryPart.GetPartMass (:1571)
    return f32(resource_mass(p) + p["mass"])


class Lcg:
    def __init__(self, seed):
        self.x = seed & 0xFFFFFFFF

    def next(self, n):
        self.x = (self.x * 1664525 + 1013904223) & 0xFFFFFFFF
        return (self.x >> 8) % n


class Inventory:
    """Python model of one ModuleInventoryPart (the pieces the numbers depend on)."""

    def __init__(self, name, slots, vol_limit, mass_limit):
        self.name, self.slots, self.vol_limit, self.mass_limit = name, slots, vol_limit, mass_limit
        self.stored = []  # DictionaryValueList: insertion-ordered [slot, part, quantity, stackCapacity]
        self.volumeOccupied = 0.0
        self.massOccupied = 0.0
        self.volumeCapacity = 0.0
        self.massCapacity = 0.0

    def slot_of(self, slot):
        for e in self.stored:
            if e[0] == slot:
                return e
        return None

    def first_empty(self):  # :1701
        for i in range(self.slots):
            if self.slot_of(i) is None:
                return i
        return -1

    def clear(self, slot):  # :847
        e = self.slot_of(slot)
        if e is not None:
            self.stored.remove(e)

    def store(self, part, slot):  # :762 (quantity 1, stackCapacity = moduleCargoStackableQuantity)
        if slot == -1:
            slot = self.first_empty()
        if slot == -1:
            return False
        if self.slot_of(slot) is not None:
            self.clear(slot)
        self.stored.append([slot, part, 1, part["stackable"]])
        return True

    def set_quantity(self, slot, how_many):  # :429
        e = self.slot_of(slot)
        if e is None:
            return False
        how_many = max(1, min(how_many, e[3]))
        e[2] = how_many
        return True

    def total_stored(self, name):  # :1862
        num = -1
        for e in reversed(self.stored):
            if e[1]["name"] == name:
                if num == -1:
                    num = 0
                num += e[2]
        return num

    def remove_n(self, name, n):  # :1881
        num = n
        if self.total_stored(name) < n:
            return False
        count = len(self.stored)
        while True:
            count -= 1
            if count >= 0:
                e = self.stored[count]
                if e[1]["name"] == name:
                    if e[2] > num:
                        e[2] -= num
                        return True
                    if e[2] == num:
                        self.clear(e[0])
                        return True
                    num -= e[2]
                    self.clear(e[0])
                continue
            return False

    def update_capacity(self):  # :1429
        self.volumeOccupied = 0.0
        self.massOccupied = 0.0
        for e in self.stored:
            p = e[1]
            self.volumeOccupied = f32(self.volumeOccupied + p["packedVolume"] * float(e[2]))
            self.massOccupied = f32(self.massOccupied + part_mass(p) * float(e[2]))
        self.volumeCapacity = self.volumeOccupied
        self.massCapacity = self.massOccupied

    def flags(self, capacity_vol=None, capacity_mass=None):  # :1461
        cv = self.volumeCapacity if capacity_vol is None else capacity_vol
        cm = self.massCapacity if capacity_mass is None else capacity_mass
        return (self.vol_limit > 0 and cv > self.vol_limit), (self.mass_limit > 0 and cm > self.mass_limit)

    def module_mass(self):  # :1928
        num = 0.0
        for e in self.stored:
            p = e[1]
            q = float(max(1, e[2]))
            num = f32(num + p["mass"] * q)  # snapshot.mass == prefab mass
            num2 = 0.0
            for r in reversed(p["resources"]):
                num2 = f32(num2 + r["density"] * f32(r["amount"]))
            num = f32(num + num2 * q)
        return num

    def module_cost(self):  # :1966 with ShipConstruction.GetPartCosts (moduleCosts == 0 for the selected parts)
        num = 0.0
        for e in self.stored:
            p = e[1]
            dry = p["cost"]
            n1 = float(dry)
            n2 = 0.0
            for r in p["resources"]:
                n1 -= r["unitCost"] * r["maxAmount"]
                n2 += r["unitCost"] * r["amount"]
            dry_f, fuel_f = f32(n1), f32(n2)
            num = f32(num + (fuel_f + dry_f) * float(max(1, e[2])))
        return num

    def has_capacity(self, p, own_mass=0.0):  # :1576 (own_mass = GetPartMass(base.part), the container's own part)
        flag = True
        if p["packedVolume"] < 0 or p["packedVolume"] + self.volumeCapacity > self.vol_limit:
            flag = False
        if flag and own_mass + self.massCapacity > self.mass_limit:
            flag = False
        return flag

    def preview(self, new_part, amount, slot_part, slot_index):  # :1508
        vc, mc = self.volumeCapacity, self.massCapacity
        pv = new_part["packedVolume"]
        if slot_part is None:
            if pv < 0:
                vc = f32(self.vol_limit * 2.0)
            else:
                vc = f32(vc + pv * float(amount))
            mc = f32(mc + part_mass(new_part) * float(amount))
        else:
            e = self.slot_of(slot_index)
            stack_amount = e[2] if e is not None else 0
            can_stack = e is not None and e[1]["name"] == new_part["name"] and e[2] < e[3]  # :413 (variant "" )
            if not can_stack:
                if pv < 0:
                    vc = f32(self.vol_limit * 2.0)
                else:
                    vc = f32(vc + pv * float(amount) - slot_part["packedVolume"] * float(stack_amount))
                mc = f32(mc + part_mass(new_part) * float(amount) - part_mass(slot_part) * float(stack_amount))
            else:
                num = float(min(amount + stack_amount, new_part["stackable"]))
                if pv < 0:
                    num2 = f32(self.vol_limit * 2.0)
                    num3 = f32(self.vol_limit * 2.0)
                else:
                    num2 = f32(pv * float(stack_amount))
                    num3 = f32(pv * num)
                vc = f32(vc + num3 - num2)
                num4 = f32(part_mass(new_part) * float(stack_amount))
                num5 = f32(part_mass(new_part) * num)
                mc = f32(mc + num5 - num4)
        return vc, mc

    def inventory_string(self):  # :1130 (ResetInventoryPartsByName + the join)
        names = []
        for i in range(self.slots):
            e = self.slot_of(i)
            names.append(e[1]["name"] if e else "")
        return ",".join(n for n in names if n)


def state(inv):
    f_v, f_m = inv.flags()
    return {
        "inventory": inv.name,
        "stored": [{"slot": e[0], "part": e[1]["name"], "quantity": e[2], "stackCapacity": e[3]} for e in inv.stored],
        "volumeOccupied": bits(inv.volumeOccupied), "massOccupied": bits(inv.massOccupied),
        "moduleMass": bits(inv.module_mass()), "moduleCost": bits(inv.module_cost()),
        "volumeCapacityReached": f_v, "massCapacityReached": f_m,
        "inventoryString": inv.inventory_string(), "emptySlots": inv.slots - len(inv.stored), "firstEmpty": inv.first_empty(),
    }


def scenario(invdefs, cargo, names, rng, label, n_ops):
    d = invdefs[label]
    inv = Inventory(label, d["slots"], d["volumeLimit"], d["massLimit"])
    ops = []
    order = list(range(inv.slots))
    for i in range(len(order) - 1, 0, -1):  # Fisher-Yates with the LCG
        j = rng.next(i + 1)
        order[i], order[j] = order[j], order[i]
    fill = max(1, min(inv.slots, 1 + rng.next(inv.slots)))
    for slot in order[:fill]:
        p = cargo[names[rng.next(len(names))]]
        inv.store(p, slot)
        ops.append({"op": "store", "slot": slot, "part": p["name"]})
        if p["stackable"] > 1:
            q = 1 + rng.next(p["stackable"])
            inv.set_quantity(slot, q)
            ops.append({"op": "setq", "slot": slot, "n": q})
    for _ in range(n_ops):
        k = rng.next(4)
        if k == 0 and inv.stored:  # replace an occupied slot
            e = inv.stored[rng.next(len(inv.stored))]
            p = cargo[names[rng.next(len(names))]]
            inv.store(p, e[0])
            ops.append({"op": "store", "slot": e[0], "part": p["name"]})
        elif k == 1 and inv.stored:
            e = inv.stored[rng.next(len(inv.stored))]
            q = rng.next(e[3] + 2)
            inv.set_quantity(e[0], q)
            ops.append({"op": "setq", "slot": e[0], "n": q})
        elif k == 2 and inv.stored:
            e = inv.stored[rng.next(len(inv.stored))]
            n = 1 + rng.next(3)
            ok = inv.remove_n(e[1]["name"], n)
            ops.append({"op": "removeN", "part": e[1]["name"], "n": n, "ok": ok})
        elif k == 3 and inv.stored:
            e = inv.stored[rng.next(len(inv.stored))]
            inv.clear(e[0])
            ops.append({"op": "clear", "slot": e[0]})
    inv.update_capacity()
    return inv, ops


def fmt_f(value_f32, decimals):
    """float.ToString("F<decimals>") on the Mono / .NET Framework runtime: the float's 7 significant digits, half-up."""
    d = Decimal(float(np.float32(value_f32)))
    d7 = Decimal(format(d, ".7e")) if d != 0 else d
    q = Decimal(1).scaleb(-decimals)
    return str(d7.quantize(q, rounding=ROUND_HALF_UP))


def main():
    resdefs = load_resources()
    cargo, invdefs = load_parts(resdefs)
    # the inventory containers used by the sweep: (cfg part name) -> limits; names that do not exist are skipped
    chosen = [n for n in ("kerbalEVA", "cargoContainer", "smallCargoContainer", "CargoStorageUnit", "ConformalStorageUnit", "mk1pod.v2") if n in invdefs]
    positive = sorted(n for n, p in cargo.items() if p["packedVolume"] > 0)
    out = {"inventories": {n: invdefs[n] for n in chosen}, "parts": [cargo[n] for n in positive]}
    for p in out["parts"]:
        p["mass_bits"] = bits(p["mass"])
        p["cost_bits"] = bits(p["cost"])
        p["packedVolume_bits"] = bits(p["packedVolume"])
        p["resourceMass_bits"] = bits(resource_mass(p))
        p["partMass_bits"] = bits(part_mass(p))
    # ---- singletons: one part, quantity 1 and full stack
    singles = []
    for n in positive:
        p = cargo[n]
        for q in sorted({1, p["stackable"]}):
            inv = Inventory("single", 1, 100000.0, 100000.0)
            inv.store(p, 0)
            inv.set_quantity(0, q)
            inv.update_capacity()
            singles.append({"part": n, "quantity": q, **state(inv)})
    out["singles"] = singles
    # ---- fills with random operations
    rng = Lcg(33017)
    fills = []
    for rep in range(6):
        for label in chosen:
            inv, ops = scenario(invdefs, cargo, positive, rng, label, 6)
            s = state(inv)
            s["ops"] = ops
            host_mass = float(np.uint32(invdefs[label]["hostMass_bits"]).view(np.float32))
            # kerbalEVA is a code-built prefab (WP-13: its Part.mass is the Unity asset's) and mk1pod.v2 is crewed (the loader subtracts the
            # per-seat mass): their host mass is not a cfg value, so HasCapacity (which reads GetPartMass(base.part)) is not sampled there
            s["hasCapacity"] = [] if label in ("kerbalEVA", "mk1pod.v2") else [{"part": n, "result": inv.has_capacity(cargo[n], host_mass)} for n in positive[:: max(1, len(positive) // 12)]]
            # previews: every (new part, amount, slot) combination of a small grid
            prev = []
            for k in range(8):
                np_ = cargo[positive[rng.next(len(positive))]]
                slot = rng.next(inv.slots)
                amount = 1 + rng.next(max(1, np_["stackable"]))
                e = inv.slot_of(slot)
                vc, mc = inv.preview(np_, amount, e[1] if e else None, slot)
                prev.append({"part": np_["name"], "slot": slot, "amount": amount, "slotPart": e[1]["name"] if e else "",
                             "volumeCapacity": bits(vc), "massCapacity": bits(mc),
                             "volReached": bool(inv.vol_limit > 0 and vc > inv.vol_limit), "massReached": bool(inv.mass_limit > 0 and mc > inv.mass_limit)})
            s["previews"] = prev
            fills.append(s)
    out["fills"] = fills
    # ---- EVA construction weight (Part.cs:4060): partWeight = (double)((cpMass + resMass) * 1000f) * gee, vs the combined limit
    phys = open(PHYSICS, encoding="utf-8").read()
    m = re.search(r"^constructionWeightLimit\s*=\s*([0-9.]+)", phys, re.M)
    limit = f32(float(m.group(1)))  # PhysicsGlobals.cs:3330 constructionWeightLimit = float.Parse(value)
    m2 = re.search(r"^constructionWeightLimitPerKerbalCombine\s*=\s*([0-9.]+)", phys, re.M)
    per_kerbal = f32(float(m2.group(1))) if m2 else 9.80665 * 60.0  # :3335, else the field default GravitationalAcceleration * 60.0 (:416)
    out["physics"] = {"constructionWeightLimit": limit, "constructionWeightLimitPerKerbalCombine": per_kerbal}
    gees = [9.80665, 3.7, 1.63, 0.6, 24.79, 1e-06]
    weights = []
    for n in sorted(cargo):
        p = cargo[n]
        if p["packedVolume"] <= 0:
            continue
        for g in gees:
            w = float(f32((p["mass"] + resource_mass(p)) * 1000.0)) * g
            row = {"part": n, "gee": g, "partWeight": w}
            for assisting in range(0, 5):
                lim = limit + float(assisting) * per_kerbal
                row["under%d" % assisting] = w <= lim
            weights.append(row)
    out["weights"] = weights
    # ---- UpdateInfoLabels text: (float)(limit / max(gee, 1e-6)) * 0.001f -> "{0:F2}t" (or the >= 100000 branch)
    labels = []
    for assisting in range(0, 5):
        for g in gees + [0.0]:
            lim = limit + float(assisting) * per_kerbal
            num = lim / max(g, 1e-06)
            if num < 100000.0:
                text = fmt_f(f32(f32(num) * 0.001), 2) + "t"
            else:
                text = "#autoLOC_8014166"
            labels.append({"assisting": assisting, "gee": g, "limit": lim, "text": text})
    out["labels"] = labels
    json.dump(out, sys.stdout, indent=1, sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
