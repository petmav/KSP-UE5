"""WP-32 reference: shared inputs. Game data (the kerbalEVA MODULE and the evaJetpack part of the GameData mirror, Physics.cfg) is read from
core/testdata/GameData-mirror; the animation table is the SYNTHETIC one the C++ fixture builds (tests/eva_fixture.hpp SetupClips): clip names
"clip<i>" in the field order of KerbalAnimationManager.cs, a jp_ variant for i % 3 == 0, length 0.5 + 0.125 * (i % 21)."""
import os
import re

from eva_math import f32

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.normpath(os.path.join(HERE, "..", ".."))
MIRROR = os.path.join(CORE, "testdata", "GameData-mirror")
CS_ROOT = os.path.normpath(os.path.join(CORE, "..", "decompiled", "KSP-Decompiled", "Assembly-CSharp"))


def read_text(path):
    with open(path, encoding="utf-8-sig") as f:
        return f.read()


def kerbal_eva_cfg():
    """key -> string of the MODULE { name = KerbalEVA } node of Squad/Parts/Prebuilt/kerbalEVA.cfg"""
    txt = read_text(os.path.join(MIRROR, "GameData", "Squad", "Parts", "Prebuilt", "kerbalEVA.cfg"))
    i = txt.index("name = KerbalEVA")
    j = txt.index("}", i)
    vals = {}
    for line in txt[i:j].split("\n"):
        line = line.split("//")[0].strip()
        if "=" in line:
            k, v = [x.strip() for x in line.split("=", 1)]
            vals[k] = v
    return vals


# the KSPField defaults of KerbalEVA.cs that the cfg does not override (KerbalEVA.cs initialisers)
CS_DEFAULTS = {
    "walkSpeed": 0.8, "strafeSpeed": 0.5, "runSpeed": 2.2, "turnRate": 4.0, "maxJumpForce": 10.0, "jumpMultiplier": 0.5, "boundForce": 1.0,
    "boundSpeed": 0.8, "boundThreshold": 0.0, "swimSpeed": 0.8, "ladderClimbSpeed": 0.6, "ladderPushoffForce": 3.0, "minWalkingGee": 0.17,
    "initialMass": 1.5, "massMultiplier": 0.03, "onFallHeightFromTerrain": 0.3, "clamberMaxAlt": 100.0, "boundFrequency": 0.15, "boundAttack": 0.4,
    "boundRelease": 2.0, "boundFallThreshold": 1.5, "Kp": 0.7, "Ki": 0.25, "Kd": 0.3, "iC": 0.005, "rotPower": 1.0, "linPower": 10.0,
    "PropellantConsumption": 0.025, "stumbleThreshold": 3.5, "recoverThreshold": 0.6, "recoverTime": 3.0, "flagReach": 0.3, "clamberReach": 0.9,
    "clamberStandoff": 0.45, "helmetOffMinSafePressureAtm": 0.177, "helmetOffMaxSafePressureAtm": 20.0, "helmetOffMinSafeTempK": 223.0,
    "helmetOffMaxSafeTempK": 333.0, "helmetOffMaxOceanPressureAtm": 5.8, "helmetOffMinSafePressureAtmMargin": 0.038,
    "helmetOffMaxSafePressureAtmMargin": 5.0, "helmetOffMinSafeTempKMargin": 10.0, "helmetOffMaxSafeTempKMargin": 10.0,
    "helmetOffMaxOceanPressureAtmMargin": 1.0, "evaExitTemperature": 303.0, "VisorAnimationSpeed": 12.0, "thrustPercentage": 100.0,
}


def cfg_float(name):
    """the float32 value the module holds after the cfg overload (cfg value if present, else the C# initialiser)"""
    cfg = kerbal_eva_cfg()
    if name in cfg:
        return f32(float(cfg[name]))
    return f32(CS_DEFAULTS[name])


def anim_fields():
    txt = read_text(os.path.join(CS_ROOT, "KerbalAnimationManager.cs"))
    return re.findall(r"public KerbalAnimationState (\w+);", txt)


def anim_table():
    """field -> dict(index, name, jp, length)"""
    t = {}
    for i, f in enumerate(anim_fields()):
        t[f] = dict(index=i, name="clip%d" % i, jp=(i % 3) == 0, length=f32(0.5 + 0.125 * (i % 21)))
    return t


def anim_string(table, field, jetpack):
    a = table[field]
    if jetpack and a["jp"]:
        return "jp_" + a["name"]
    return a["name"]
