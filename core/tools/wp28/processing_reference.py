#!/usr/bin/env python3
"""WP-28: independent reference for the resource-processing catch-up maths -- test data generator.

Re-implements, in plain Python and from the decompiled C#, the numeric core of
  BaseConverter.GetDeltaTime / GetBestDeltaTime / FixedUpdate (BaseConverter.cs:226, :264, :500),
  BaseConverter.GetEfficiencyMultiplier / GetCrewEfficiencyBonus / HasSpecialist / TallyEfficiencyModifiers (:493, :108, :671, :568),
  ModuleResourceConverter.PrepareRecipe / LoadRecipe (ModuleResourceConverter.cs:15, :25) incl. ConvertRecipeToUnits (BaseConverter.cs:578),
  ResourceConverter.ProcessRecipe (ResourceConverter.cs:23),
  ModuleSpaceObjectInfo.SetupSpaceObjectResources (ModuleSpaceObjectInfo.cs:62) with KSPRandom (KSPRandom.cs).
It shares NO code with kspcore. Float32 stores use numpy.float32 per the Mono float rule (a float expression is evaluated in double and
rounded only where the C# stores / passes / returns it as a float).

The resource store is a simple deterministic tank model the C++ test mirrors with its own IResourceBroker (so the reference does not need
the PartSet solver): every resource has (amount, max, density);
  AmountAvailable = amount;  StorageAvailable(fill) = max(0, max * fill - amount);
  RequestResource(r > 0) removes min(r, amount) and returns it;  StoreResource(r) adds min(r, max - amount).
Part resources read directly by ProcessRecipe's requirement loop are the same tanks.

Usage: python tools/wp28/processing_reference.py > tests/data/wp28_processing_reference.json
"""
import json
import math
import random
import sys

import numpy as np

INT_MAX = 2147483647


def st(x):
    """Round to float32 (a C# float store); return the exact value as a Python float."""
    return float(np.float32(x))


def i32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


# ---- KSPRandom.cs ---------------------------------------------------------------------------------------------------------
class KSPRandom:
    def __init__(self, seed):
        self.a = [0] * 56
        num = i32(161803398 - abs(seed))
        self.a[55] = num
        num2 = 1
        for i in range(1, 55):
            n3 = 21 * i % 55
            self.a[n3] = num2
            num2 = i32(num - num2)
            if num2 < 0:
                num2 = i32(num2 + INT_MAX)
            num = self.a[n3]
        for _ in range(1, 5):
            for k in range(1, 56):
                self.a[k] = i32(self.a[k] - self.a[1 + (k + 30) % 55])
                if self.a[k] < 0:
                    self.a[k] = i32(self.a[k] + INT_MAX)
        self.ia = 0
        self.ib = 31

    def sample(self):
        self.ia += 1
        if self.ia >= 56:
            self.ia = 1
        self.ib += 1
        if self.ib >= 56:
            self.ib = 1
        num = i32(self.a[self.ia] - self.a[self.ib])
        if num < 0:
            num = i32(num + INT_MAX)
        self.a[self.ia] = num
        return num * 4.6566128752458e-10

    def next_max(self, maxv):  # Next(int maxValue)
        return int(math.trunc(self.sample() * float(maxv)))

    def next_range(self, lo, hi):  # Next(int minValue, int maxValue)
        num = (hi - lo) & 0xFFFFFFFF
        if num <= 1:
            return lo
        return i32((int(math.trunc(self.sample() * float(num))) & 0xFFFFFFFF) + lo)

    def next_double_range(self, lo, hi):  # NextDouble(double, double)
        num = hi - lo
        if num <= 1.0:
            return lo
        return self.sample() * num + lo


# ---- the tank model ---------------------------------------------------------------------------------------------------------
class Tanks:
    def __init__(self, tanks):
        self.t = {k: {"amount": v["amount"], "max": v["max"], "density": v["density"]} for k, v in tanks.items()}

    def avail(self, name):
        return self.t[name]["amount"]

    def storage(self, name, fill):
        t = self.t[name]
        return max(0.0, t["max"] * fill - t["amount"])

    def request(self, name, amount):
        t = self.t[name]
        take = min(amount, t["amount"]) if amount > 0 else 0.0
        t["amount"] -= take
        return take

    def store(self, name, amount):
        t = self.t[name]
        put = min(amount, t["max"] - t["amount"]) if amount > 0 else 0.0
        t["amount"] += put
        return put


def ratio_from_cfg(r):
    """ResourceRatio.Load: `Ratio = float.Parse(..)` (a float widened to the double field)."""
    return st(r)


# ---- BaseConverter -------------------------------------------------------------------------------------------------------------
def convert_recipe_to_units(recipe, tanks):
    """BaseConverter.ConvertRecipeToUnits (:578), including the quirk that a dense requirement is added to the OUTPUT list."""
    ins, outs, reqs = [], [], []
    for r in recipe["inputs"]:
        d = tanks.t[r["name"]]["density"]
        ins.append(dict(r, ratio=r["ratio"] / d) if d > 1e-09 else dict(r))
    for r in recipe["outputs"]:
        d = tanks.t[r["name"]]["density"]
        outs.append(dict(r, ratio=r["ratio"] / d) if d > 1e-09 else dict(r))
    for r in recipe["reqs"]:
        d = tanks.t[r["name"]]["density"]
        if d > 1e-09:
            outs.append(dict(r, ratio=r["ratio"] / d))  # sic: list2
        else:
            reqs.append(dict(r))
    return {"inputs": ins, "outputs": outs, "reqs": reqs}


def has_specialist(crew, effect):
    num = -1
    i = len(crew)
    while i > 0:
        i -= 1
        m = crew[i]
        if effect in m["effects"] and m["level"] > num:
            num = m["level"]
        if num == 5:
            break
    return num


def crew_bonus(sc):
    result = 1.0
    if sc["useSpecialistBonus"]:
        spec = float(has_specialist(sc["crew"], "ConverterSkill"))
        result = st(sc["specialistBonusBase"] + (1.0 + spec) * sc["specialistEfficiencyFactor"])
    return result


def efficiency_multiplier(sc, activated=True):
    heat = st(sc["thermalEfficiency"]) if activated else 1.0
    total = 1.0
    for v in sc["modifiers"]:
        total = st(total * v)
    # EfficiencyBonus * heatThrottle * _totalEfficiencyModifiers * GetCrewEfficiencyBonus(): float operands, double product
    return sc["efficiencyBonus"] * heat * total * crew_bonus(sc), total


def process_recipe(tanks, dt, recipe, fill, take, eff_bonus_f32):
    """ResourceConverter.ProcessRecipe (ResourceConverter.cs:23): returns TimeFactor."""
    num = 1.0
    for r in recipe["reqs"]:
        num3 = tanks.t[r["name"]]["amount"] / abs(r["ratio"])
        if r["ratio"] < 0.0:
            num3 = 1.0 - num3
        if num3 < num:
            num = num3
        if not (num > 1e-09):
            return 0.0
    num *= eff_bonus_f32
    if num <= 1e-09:
        return 0.0
    num4 = dt
    for r in recipe["inputs"]:
        num5 = tanks.avail(r["name"]) * take
        num6 = r["ratio"] * num4 * num
        if not (num5 < num6):
            continue
        num4 *= num5 / num6
        if not (num4 > 1e-09):
            return 0.0
    for r in recipe["outputs"]:
        if not r["dump"]:
            num8 = tanks.storage(r["name"], fill)
            num9 = r["ratio"] * num4 * num
            if num8 < num9:
                num4 *= num8 / num9
                if not (num4 > 1e-09):
                    return 0.0
    for r in recipe["inputs"]:
        tanks.request(r["name"], r["ratio"] * num4 * num)
    for r in recipe["outputs"]:
        tanks.store(r["name"], r["ratio"] * num4 * num)
    return num4 * num


def best_delta_time(recipe, tanks, dt):
    num = dt
    for r in recipe["inputs"]:
        num2 = tanks.avail(r["name"])
        num3 = dt * (num2 / (r["ratio"] * dt))
        if num3 < num:
            num = num3
    for r in recipe["outputs"]:
        num4 = tanks.storage(r["name"], 1.0)
        num5 = dt * (num4 / (r["ratio"] * dt))
        if num5 < num:
            num = num5
    return num


def run_scenario(sc):
    tanks = Tanks(sc["tanks"])
    recipe = {"inputs": sc["inputs"], "outputs": sc["outputs"], "reqs": sc["reqs"]}
    if sc["convertByMass"]:
        recipe = convert_recipe_to_units(recipe, tanks)
    fixed_dt = st(sc["fixedDeltaTime"])
    last_update = 0.0
    retries = 0
    last_tf = 0.0
    activated = False  # IsActivated: AlwaysActive sets it inside FixedUpdate only after GetDeltaTime (BaseConverter.cs:500), so the
    # first catch-up step runs with IsActivated == false and ModuleResourceConverter.PrepareRecipe (:15) returns null
    steps = []
    for ut in sc["uts"]:
        # GetDeltaTime
        if abs(last_update) < 1e-09:
            last_update = ut
            dt = -1.0
        else:
            num = min(ut - last_update, float(sc["maxDeltaTime"]))
            num2 = min(fixed_dt, 0.02)
            if num > num2 and retries < 10:
                best = best_delta_time(recipe, tanks, num) if activated else num  # GetBestDeltaTime: recipe == null -> deltaTime
                if best < num:
                    retries += 1
                    num = max(num2, best)
            last_update += num
            dt = num
        rec = {"ut": ut, "dt": dt}
        if dt >= 1e-09:
            activated = True
            eff, _ = efficiency_multiplier(sc)
            tf = process_recipe(tanks, dt, recipe, sc["fillAmount"], sc["takeAmount"], st(eff))
            num2 = tf / dt
            if abs(num2 - last_tf) > 1e-09:
                last_tf = num2
            rec["efficiency"] = eff
            rec["timeFactor"] = tf
        rec["lastUpdateTime"] = last_update
        rec["catchupRetries"] = retries
        rec["lastTimeFactor"] = last_tf
        rec["amounts"] = {k: v["amount"] for k, v in tanks.t.items()}
        steps.append(rec)
    return steps


def make_scenarios():
    rnd = random.Random(28)
    scs = []

    def rf(lo, hi):
        return st(rnd.uniform(lo, hi))

    # tank set shared by every scenario: names are real stock resources (densities from the game's resource configs, read by the test)
    for n in range(48):
        nin = rnd.choice([1, 1, 2])
        nout = rnd.choice([1, 2])
        tanks = {
            "Ore": {"amount": rf(0, 400), "max": 400.0, "density": st(0.01)},
            "ElectricCharge": {"amount": rf(0, 1000), "max": 1000.0, "density": 0.0},
            "LiquidFuel": {"amount": rf(0, 200), "max": 400.0, "density": st(0.005)},
            "Oxidizer": {"amount": rf(0, 200), "max": 450.0, "density": st(0.005)},
            "MonoPropellant": {"amount": rf(0, 100), "max": 200.0, "density": st(0.004)},
        }
        names_in = ["Ore", "ElectricCharge"][:nin]
        names_out = ["LiquidFuel", "Oxidizer", "MonoPropellant"][:nout]
        inputs = [{"name": nm, "ratio": ratio_from_cfg(rnd.choice([0.5, 1.0, 2.5, 30.0, rnd.uniform(0.01, 20)])), "dump": False} for nm in names_in]
        outputs = [{"name": nm, "ratio": ratio_from_cfg(rnd.uniform(0.01, 5.0)), "dump": rnd.random() < 0.25} for nm in names_out]
        reqs = []
        if rnd.random() < 0.3:
            reqs.append({"name": "ElectricCharge", "ratio": ratio_from_cfg(rnd.choice([10.0, 50.0, -500.0])), "dump": False})
        nmods = rnd.choice([0, 0, 1, 3])
        crew = [{"effects": ["ConverterSkill"] if rnd.random() < 0.7 else ["PilotSkill"], "level": rnd.randint(0, 5)} for _ in range(rnd.choice([0, 1, 3]))]
        uts = [1000.0]
        t = 1000.0
        for _ in range(rnd.choice([6, 9])):
            t += rnd.choice([0.02, 0.02, 0.04, 0.5, 30.0, 5000.0, 25000.0, 100000.0])
            uts.append(t)
        sc = {
            "name": "scenario%d" % n,
            "tanks": tanks,
            "inputs": inputs,
            "outputs": outputs,
            "reqs": reqs,
            "convertByMass": rnd.random() < 0.4,
            "efficiencyBonus": rf(0.2, 1.5),
            "thermalEfficiency": rf(0.1, 1.0),
            "modifiers": [rf(0.5, 1.2) for _ in range(nmods)],
            "useSpecialistBonus": rnd.random() < 0.5,
            "specialistBonusBase": st(0.05),
            "specialistEfficiencyFactor": st(0.1),
            "crew": crew,
            "fillAmount": st(rnd.choice([1.0, 0.9, 0.5])),
            "takeAmount": st(rnd.choice([1.0, 1.0, 0.75])),
            "fixedDeltaTime": 0.02,
            "maxDeltaTime": rnd.choice([21600, 600, 100000]),
            "uts": uts,
        }
        sc["expected"] = run_scenario(sc)
        scs.append(sc)
    return scs


# ---- ModuleSpaceObjectInfo.SetupSpaceObjectResources -----------------------------------------------------------------------------
def setup_space_object(seed, current_mass, resources):
    rng = KSPRandom(seed)
    total_pct = 0.0
    total_mass = 0.0
    data = []
    for r in resources:
        if rng.next_max(100) < r["presenceChance"]:
            min_v = float(r["lowRange"]) * 0.01
            max_v = float(r["highRange"]) * 0.01
            num = rng.next_double_range(min_v, max_v)
            if total_pct + num > 1.0:
                num = 1.0 - total_pct
            total_pct += num
            mass = st(current_mass * num)
            purity = st(st(float(rng.next_range(r["lowRange"], r["highRange"]))) * st(0.01))
            data.append({"name": r["name"], "mass": mass, "purity": purity})
            total_mass = st(total_mass + mass)
    out = []
    for r in resources:
        ab = None
        for d in data:
            if d["name"] == r["name"]:
                val = st(d["mass"] / total_mass)
                ab = st(max(st(0.01), val) * d["purity"])
        out.append(ab)
    if total_mass > current_mass:
        current_mass = total_mass * st(1.01)  # float * float passed into a double setter: no float store
    threshold = current_mass - total_mass
    return {"abundances": out, "totalResourceMass": total_mass, "totalResourcePercent": total_pct, "currentMass": current_mass, "massThreshold": threshold}


def make_space_objects():
    rnd = random.Random(2828)
    cases = []
    for n in range(40):
        seed = rnd.randint(1, 2 ** 31 - 1)
        mass = float(rnd.choice([1, 5, 12, 40, 250]) + rnd.random())
        res = []
        for nm in ["Ore", "Rock", "Metal", "Ice", "Gold"][: rnd.choice([2, 3, 5])]:
            lo = rnd.randint(0, 20)
            hi = lo + rnd.randint(0, 60)
            res.append({"name": nm, "presenceChance": rnd.choice([0, 30, 70, 100]), "lowRange": lo, "highRange": hi})
        cases.append({"seed": seed, "currentMass": mass, "resources": res, "expected": setup_space_object(seed, mass, res)})
    return cases


def main():
    json.dump({"scenarios": make_scenarios(), "spaceObjects": make_space_objects()}, sys.stdout, indent=1)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
