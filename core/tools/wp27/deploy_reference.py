#!/usr/bin/env python3
"""WP-27: independent reference for the parachute drag / heating maths and the solar-panel flux maths -- test data generator.

Re-implements, in plain Python and from the decompiled C#, the numeric core of
  ModuleParachute.GetArea / CalcBaseStats / SetConvectiveStats / DeploySafe / UpdateCut (ModuleParachute.cs:522, :537, :939, :960, :985)
  and the drag lerps of FixedUpdate (ModuleParachute.cs:610: Mathf.Pow(normalizedTime, deploymentCurve), Mathf.Lerp),
  ModuleDeployableSolarPanel.PostCalculateTracking (ModuleDeployableSolarPanel.cs:307: sunAOA, _distMult, _efficMult, _flowRate),
  ModuleDeployablePart.SetDragCubes (ModuleDeployablePart.cs:1308).
It shares NO code with kspcore. Float32 stores use numpy.float32 per the Mono float rule (a float expression is evaluated in double and rounded
only where the C# stores / passes / returns it as a float).

Inputs (all original game data, never hand-authored):
  harness/captures/out2/part_database.json   module_fields (every KSPField of every module of every stock part, as the game resolved it) and
                                             drag_cubes (the game's `saved` cube strings)
  harness/golden/serialized/physicsglobals.serialized.json   the PhysicsGlobals prefab (drag curves, constants)
  harness/inputs/Physics.cfg                 the game's Physics.cfg (dragMultiplier, dragCubeMultiplier, space temperature, ...)
Curve evaluation (FloatCurve.Evaluate = AnimationCurve) is the cubic Hermite of Unity's native code written here from its documented form; the
tests compare curve-derived values with a small ULP allowance and everything else exactly.

Usage: python -I core/tools/wp27/deploy_reference.py > core/tests/data/wp27_reference.json
"""
import json
import math
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
F = np.float32


def f32(x):
    return F(x)


def st(x):
    """Mono float store: round a double expression to float32."""
    return float(F(x))


def hexf(x):
    return "%08x" % int(np.array([x], dtype=np.float32).view(np.uint32)[0])


# ---------------------------------------------------------------- data
def load_capture():
    d = json.load(open(os.path.join(ROOT, "harness", "captures", "out2", "part_database.json")))
    T = {t["name"]: t for t in d["tables"]}
    return T


def table_rows(t):
    cols = [c["name"] for c in t["columns"]]
    return [dict(zip(cols, r)) for r in t["rows"]]


def module_values(T, part, cls):
    """{module_index: {field: value}} of the modules of `cls` on `part` (numbers parsed, strings kept)."""
    mods = [r for r in table_rows(T["modules"]) if r["part"] == part and r["class"] == cls]
    out = {}
    for m in mods:
        out[m["module_index"]] = {}
    for r in table_rows(T["module_fields"]):
        if r["part"] == part and r["module_index"] in out:
            v = r["value"]
            ty = r["type"]
            if ty == "Single":
                v = float(F(float(v)))
            elif ty == "Double":
                v = float(v)
            elif ty in ("Int32", "Int64", "UInt32"):
                v = int(float(v))
            elif ty == "Boolean":
                v = v == "True"
            out[r["module_index"]][r["field"]] = v
    return out


def cubes_of(T, part):
    cubes = {}
    for r in table_rows(T["drag_cubes"]):
        if r["part"] == part:
            toks = [t.strip() for t in r["saved"].split(",")]
            name = toks[0]
            nums = [float(F(float(x))) for x in " ".join(toks[1:]).split()]
            # 6 x (area, drag, depth), center xyz, size xyz
            area = [nums[i * 3] for i in range(6)]
            drag = [nums[i * 3 + 1] for i in range(6)]
            cubes[name] = {"area": area, "drag": drag}
    return cubes


def physics():
    pg = json.load(open(os.path.join(ROOT, "harness", "golden", "serialized", "physicsglobals.serialized.json")))["PhysicsGlobals"]

    def key0(curve):
        k = pg[curve]["fCurve"]["m_Curve"][0]
        assert k["time"] == 0.0
        return float(F(k["value"]))

    cfg = open(os.path.join(ROOT, "harness", "inputs", "Physics.cfg")).read()

    def c(name):
        m = re.search(r"^%s\s*=\s*([^\s/]+)" % name, cfg, re.M)
        return float(m.group(1))

    return {
        "tail0": key0("dragCurveTail"), "surface0": key0("dragCurveSurface"), "tip0": key0("dragCurveTip"), "mult0": key0("dragCurveMultiplier"),
        "dragMultiplier": float(F(c("dragMultiplier"))), "dragCubeMultiplier": float(F(c("dragCubeMultiplier"))),
        "spaceTemperature": c("spaceTemperature"), "newtonianTemperatureFactor": c("newtonianTemperatureFactor"),
        "radiationFactor": c("radiationFactor"), "solarLuminosityAtHome": c("solarLuminosityAtHome"),
        "fullToCrossSectionLerpStart": c("fullToCrossSectionLerpStart"), "fullToCrossSectionLerpEnd": c("fullToCrossSectionLerpEnd"),
        "stefanBoltzmanConstant": 5.670373e-08,  # PhysicsGlobals.cs:103
    }


# ---------------------------------------------------------------- Mathf
def mf_lerp(a, b, t):
    t = min(max(t, 0.0), 1.0)
    return st(float(a) + (float(b) - float(a)) * float(t))


def mf_pow(f, p):
    return st(math.pow(float(f), float(p)))


def clamp01(x):
    return min(max(x, 0.0), 1.0)


def util_lerp(a, b, t):
    t = clamp01(t)
    return (1.0 - t) * a + t * b


# ---------------------------------------------------------------- ModuleParachute
FACE_DIRS = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]  # DragCubeList.faceDirections (XP XN YP YN ZP ZN)


def dot3(a, b):
    # Vector3.Dot returns float: a.x*b.x + a.y*b.y + a.z*b.z evaluated in double, stored (all components are 0 / +-1: exact)
    return st(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])


def drag_curve_value(pg, dot_norm, mach=0.0):
    """PhysicsGlobals.DragCurveValue at mach 0: every curve's first key is at time 0, so Evaluate(0) is the key value."""
    if dot_norm <= 0.5:
        a, b, num = pg["tail0"], pg["surface0"], pg["mult0"]
        return st(mf_lerp(a, b, st(dot_norm * 2.0)) * num)
    a2, b2, num2 = pg["surface0"], pg["tip0"], pg["mult0"]
    return st(mf_lerp(a2, b2, st((dot_norm - 0.5) * 2.0)) * num2)


def get_area(pg, cube):
    face = FACE_DIRS[3]  # YN
    num = 0.0
    for i in range(6):
        rhs = FACE_DIRS[i]
        dotn = st((dot3(face, rhs) + 1.0) * 0.5)
        num2 = drag_curve_value(pg, dotn)
        num3 = st(cube["area"][i] * num2)
        num = st(num + num3 * cube["drag"][i])
    return float(num) * float(pg["dragCubeMultiplier"]) * float(pg["dragMultiplier"])


class Chute:
    def __init__(self, pg, f, cubes):
        self.pg, self.f = pg, f
        self.areaDeployed = get_area(pg, cubes["DEPLOYED"])
        self.areaSemi = get_area(pg, cubes["SEMIDEPLOYED"])
        self.machHeatMult = self.convectivekW = self.shockTemp = self.convectionArea = 0.0
        self.chuteTemp = f["startingTemp"]
        self.invThermalMass = 1.0 / (self.areaDeployed * f["chuteThermalMassPerArea"])

    def set_convective_stats(self, density, extTemp, mach, convCoeff):
        f, pg = self.f, self.pg
        if density > 0.0:
            self.convectionArea = self.areaDeployed
            self.machHeatMult = math.pow(f["machHeatMultBase"] + mach, f["machHeatMultPow"])
            start = pg["fullToCrossSectionLerpStart"]
            self.machHeatMult = util_lerp(1.0, self.machHeatMult, (mach * f["machHeatMultScalar"] - start) / (pg["fullToCrossSectionLerpEnd"] - start))
            self.machHeatMult = util_lerp(1.0, self.machHeatMult, density * f["machHeatDensityFadeoutMult"])
            self.convectivekW = 0.001 * convCoeff * self.convectionArea * self.machHeatMult
            self.shockTemp = extTemp * self.machHeatMult
        else:
            self.convectivekW = 0.0
            self.shockTemp = pg["spaceTemperature"]

    def calc_base_stats(self):
        f, pg = self.f, self.pg
        num, num2, num3, flag = 0.0, 401, 100.0, True
        num4 = 1.0 / f["refSpeedOfSound"]
        num5 = f["chuteMaxTemp"] * f["safeMult"]
        self.set_convective_stats(f["refDensity"], num * pg["newtonianTemperatureFactor"], num * num4, 0.0)
        while True:
            if abs(self.shockTemp - num5) <= 0.1:
                break
            num2 -= 1
            if num2 < 0:
                break
            if self.shockTemp > num5:
                if flag:
                    num3 *= 0.5
                flag = False
                num -= num3
                if num < 0.0:
                    num = 0.0
            else:
                if not flag:
                    num3 *= 0.5
                flag = True
                num += num3
            self.set_convective_stats(f["refDensity"], num * pg["newtonianTemperatureFactor"], num * num4, 0.0)
        return round(num)  # Math.Round (banker's): python's round() on a float is the same

    def deploy_safe(self, ut, timeUnpacked):
        f = self.f
        if ut - timeUnpacked < 1.0:
            return "UNSAFE"
        if self.shockTemp <= f["chuteMaxTemp"] * f["safeMult"]:
            return "SAFE"
        num = (self.shockTemp - self.chuteTemp) * clamp01(self.convectivekW * self.invThermalMass * f["secondsForRisky"])
        finalTemp = self.chuteTemp + num
        if finalTemp <= f["chuteMaxTemp"]:
            return "RISKY"
        return "UNSAFE"

    def update_cut_heat(self, fdt):
        """The heating branch of UpdateCut (ModuleParachute.cs:1073): returns True when the chute overheats and is cut."""
        f, pg = self.f, self.pg
        if self.chuteTemp < pg["spaceTemperature"]:
            self.chuteTemp = f["startingTemp"]
        num = self.chuteTemp
        num2 = 0.001 * pg["stefanBoltzmanConstant"] * self.areaDeployed * f["chuteEmissivity"] * pg["radiationFactor"]
        num3 = self.convectivekW * (self.shockTemp - self.chuteTemp)
        num4 = min(self.chuteTemp, f["chuteMaxTemp"])
        num4 *= num4
        num3 -= num2 * num4 * num4
        self.chuteTemp += num3 * self.invThermalMass * fdt
        num3 = self.convectivekW * (self.shockTemp - self.chuteTemp)
        num4 = min(self.chuteTemp, f["chuteMaxTemp"])
        num4 *= num4
        num3 -= num2 * num4 * num4
        self.chuteTemp += num3 * self.invThermalMass * fdt
        self.chuteTemp = (num + self.chuteTemp) * 0.5
        if math.isnan(self.chuteTemp) or self.chuteTemp < pg["spaceTemperature"]:
            self.chuteTemp = pg["spaceTemperature"]
        return self.chuteTemp > f["chuteMaxTemp"]


def parachute_cases(T, pg):
    out = []
    parts = ["parachuteSingle", "parachuteRadial", "parachuteLarge", "radialDrogue", "parachuteDrogue"]
    for part in parts:
        mv = module_values(T, part, "ModuleParachute")
        if not mv:
            continue
        f = list(mv.values())[0]
        cubes = cubes_of(T, part)
        if "DEPLOYED" not in cubes or "SEMIDEPLOYED" not in cubes:
            continue
        ch = Chute(pg, f, cubes)
        case = {"part": part, "areaDeployed": ch.areaDeployed, "areaSemi": ch.areaSemi, "invThermalMass": ch.invThermalMass,
                "maxSafeSpeedAtRef": ch.calc_base_stats()}
        # SetConvectiveStats at several flight points
        pts = []
        for (dens, ext, mach, cc) in [(1.2, 300.0, 0.3, 40.0), (0.5, 250.0, 0.9, 25.0), (0.05, 220.0, 2.0, 5.0), (0.0, 4.0, 3.0, 0.0), (1.0, 288.0, 0.0, 0.0)]:
            ch.set_convective_stats(dens, ext, mach, cc)
            pts.append({"density": dens, "extTemp": ext, "mach": mach, "conv": cc, "machHeatMult": ch.machHeatMult, "convectivekW": ch.convectivekW,
                        "shockTemp": ch.shockTemp, "safe": ch.deploy_safe(100.0, 50.0), "unsafeJustUnpacked": ch.deploy_safe(100.0, 99.5)})
        case["points"] = pts
        # heating time series: 6000 steps of dt = 0.02 s at fixed flight points (a hot one that cuts the chute, a mild one that does not);
        # chuteTemp recorded every 1000 steps and the step of the first overheat cut
        case["heats"] = []
        for (dens, ext, mach, cc) in [(0.8, 280.0, 1.6, 80.0), (1.0, 250.0, 0.5, 6.0), (0.3, 230.0, 1.0, 12.0)]:
            ch2 = Chute(pg, f, cubes)
            ch2.set_convective_stats(dens, ext, mach, cc)
            series, cut_step = [], -1
            for step in range(1, 6001):
                if ch2.update_cut_heat(float(F(0.02))) and cut_step < 0:
                    cut_step = step
                if step % 1000 == 0:
                    series.append(ch2.chuteTemp)
            case["heats"].append({"density": dens, "extTemp": ext, "mach": mach, "conv": cc, "dt": float(F(0.02)), "series": series, "cutStep": cut_step,
                                  "shockTemp": ch2.shockTemp, "convectivekW": ch2.convectivekW})
        # drag lerps of FixedUpdate (float32): lerpTime = Pow(normalizedTime, deploymentCurve); maximum_drag = baseDrag + Lerp(a, b, lerpTime)
        base = float(F(0.02))
        lerps = []
        for t in [0.0, 0.1, 0.25, 0.5, 0.75, 0.999, 1.0]:
            lt = mf_pow(t, f["deploymentCurve"])
            semi = st(base + mf_lerp(f["stowedDrag"], f["semiDeployedDrag"], lt))
            full = st(base + mf_lerp(f["semiDeployedDrag"], f["fullyDeployedDrag"], lt))
            lerps.append({"t": t, "lerpTime": lt, "semi": semi, "full": full, "wPackedSemi": st(1.0 - lt), "wSemiFull": st(1.0 - lt)})
        case["baseDrag"] = base
        case["lerps"] = lerps
        case["fields"] = {k: f[k] for k in ["stowedDrag", "semiDeployedDrag", "fullyDeployedDrag", "deploymentCurve", "chuteMaxTemp", "safeMult", "deployAltitude",
                                           "minAirPressureToOpen", "autoCutSpeed"]}
        out.append(case)
    return out


# ---------------------------------------------------------------- solar panel
class Curve:
    """Unity AnimationCurve (cubic Hermite per segment, constant extrapolation) over (time, value, inSlope, outSlope) keys, float32 per operation."""

    def __init__(self, keys):
        self.k = [(F(a), F(b), F(c), F(d)) for (a, b, c, d) in keys]

    def evaluate(self, x):
        x = F(x)
        k = self.k
        if x <= k[0][0]:
            return float(k[0][1])
        if x >= k[-1][0]:
            return float(k[-1][1])
        i = 0
        while not (k[i][0] <= x < k[i + 1][0]):
            i += 1
        t0, v0, _, m0 = k[i]
        t1, v1, m1, _ = k[i + 1]
        dt = F(t1 - t0)
        t = F((x - t0) / dt)
        t2 = F(t * t)
        t3 = F(t2 * t)
        a = F(F(F(F(2) * t3) - F(F(3) * t2)) + F(1))
        b = F(F(t3 - F(F(2) * t2)) + t)
        c = F(t3 - t2)
        d = F(F(F(-2) * t3) + F(F(3) * t2))
        return float(F(F(F(a * v0) + F(F(b * m0) * dt)) + F(F(F(c * m1) * dt) + F(d * v1))))


TEMP_EFFIC = Curve([(4.0, 1.2, 0.0, -0.0006), (300.0, 1.0, -0.0008, -0.0008), (1200.0, 0.134, -0.00035, -0.00035), (1900.0, 0.02, -3.72e-05, -3.72e-05),
                    (2500.0, 0.01, 0.0, 0.0)])  # ModuleDeployableSolarPanel.cs:97 OnAwake defaults
TIME_EFFIC = Curve([(0.0, 1.0, 0.0, 0.0)])
INV_PI = float(F(1.0) / F(3.14159274))  # 1f / (float)Math.PI folded in float


def solar_cases(T, pg):
    out = []
    parts = ["solarPanelOX10C", "solarPanelSP10C", "solarPanelOX10L", "solarPanelSP10L", "solarPanels2", "solarPanels4", "solarPanels1", "solarPanels3",
             "largeSolarPanel", "solarPanels5", "batteryBankMini"]
    for part in parts:
        mv = module_values(T, part, "ModuleDeployableSolarPanel")
        if not mv:
            continue
        f = list(mv.values())[0]
        samples = []
        for (typ, dot, flux, skin, utdays, eff, sub) in [
            ("FLAT", 1.0, 1360.0, 300.0, 0.0, 1.0, 0.0), ("FLAT", 0.5, 1360.0, 310.0, 864000.0, 1.0, 0.0), ("FLAT", 0.2, 680.0, 700.0, 8640000.0, 0.9, 0.0),
            ("FLAT", -0.3, 1360.0, 300.0, 1.0, 1.0, 0.0), ("FLAT", 1.7, 3000.0, 4.0, 0.0, 1.0, 0.0), ("FLAT", 1.0, 1360.0, 1500.0, 0.0, 1.0, 0.0),
            ("CYLINDRICAL", 0.3, 1360.0, 300.0, 5.0, 1.0, 0.0), ("CYLINDRICAL", 1.0, 1360.0, 300.0, 5.0, 1.0, 0.0), ("SPHERICAL", 0.9, 1360.0, 300.0, 5.0, 1.0, 0.0),
            ("FLAT", 1.0, 1360.0, 300.0, 0.0, 1.0, 0.4)]:
            if typ == "FLAT":
                sunAOA = st(min(max(dot, 0.0), 1.0))
            elif typ == "CYLINDRICAL":
                sunAOA = st((1.0 - abs(dot)) * INV_PI)
            else:
                sunAOA = 0.25
            tevl = TEMP_EFFIC.evaluate(st(skin))
            timeev = TIME_EFFIC.evaluate(st(utdays * 1.1574074074074073e-05))
            effic = float(F(tevl)) * float(F(timeev)) * float(F(eff))
            distMult = flux / pg["solarLuminosityAtHome"]
            flow = float(F(sunAOA)) * effic * distMult
            samples.append({"type": typ, "dot": dot, "solarFlux": flux, "skinTemperature": skin, "ut_minus_launch_seconds": utdays, "efficiencyMult": eff,
                            "sunAOA": sunAOA, "tempEffic": tevl, "efficMult": effic, "distMult": distMult, "flowRate": flow})
        out.append({"part": part, "chargeRate": f.get("chargeRate"), "flowMult": f.get("flowMult"), "samples": samples})
    return out


def drag_weight_cases():
    """ModuleDeployablePart.SetDragCubes (float32): the three weights for (retracted, angle) pairs, tracking and non-tracking."""
    out = []
    for retracted in [0.0, 0.25, 1.0 / 3.0, 0.5, 0.9, 1.0]:
        for angle in [0.0, 0.1, 0.5, 0.77, 1.0]:
            r, a = float(F(retracted)), float(F(angle))
            out.append({"retracted": r, "angle": a, "A": st((1.0 - r) * a), "B": st((1.0 - r) * (1.0 - a)), "Aonly": st(1.0 - r)})
    return out


def main():
    T = load_capture()
    pg = physics()
    out = {"physics": pg, "parachutes": parachute_cases(T, pg), "solar": solar_cases(T, pg), "dragWeights": drag_weight_cases()}
    json.dump(out, sys.stdout, indent=1)


if __name__ == "__main__":
    main()
