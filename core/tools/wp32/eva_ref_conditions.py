"""WP-32 reference: the event conditions and OnEvent target decisions of KerbalEVA.SetupFSM (KerbalEVA.cs:3767-4351), evaluated over input grids.

A "world" is a dict of the scalar inputs the conditions read; the C++ test applies the same dict to a live KerbalEVA / Vessel / engine and calls the
event's OnCheckCondition delegate (or runs RunEvent and reads the target state). The predicates below are translations of the C# lambdas; float
comparisons follow the Mono rule (comparisons happen in double; `(double)f` casts are exact widenings).
"""
from eva_common import cfg_float
from eva_math import F3, ROTATIONS, axes, bits, dot, f32, magnitude, normalize, sign, sqr_magnitude, v_approx_zero, vmul
from eva_ref_motion import LCG, b3, rnd_vec

HALF_HEIGHT = f32(0.269 - float(f32(0.01)) * 2.0)   # halfHeight -= Physics.defaultContactOffset * 2f (:3817)


def mf(name):
    return cfg_float(name)


def key(w, name, q):
    return bool(w["keys"].get(name, {}).get(q, False))


def zero(v):
    return v_approx_zero(v)


def surface_contact(w):
    return w["landed"]


def surface_or_splashed(w):
    return w["landed"] or w["splashed"]


def under_control(w):
    return w["active"] and not w["packed"]


def tstate(w):
    return float(w["t"])   # (double)Time.time - TimeAtStateEnter with TimeAtStateEnter = 0


# ---- predicates: event name -> function(world) -> bool -----------------------------------------------------------------------------------------
PRED = {
    "Hdg Acquire Start": lambda w: abs(w["dh"]) > 60.0,
    "Hdg Acquire Complete": lambda w: abs(w["dh"]) < 30.0,
    "Move (Arcade)": lambda w: (not zero(w["tgt"])) and w["gee"] > float(mf("minWalkingGee")) and not w["cf"] and not w["restricted"] and surface_contact(w),
    "Move (FPS)": lambda w: (not zero(w["tgt"])) and w["gee"] > float(mf("minWalkingGee")) and w["cf"] and not w["restricted"] and surface_contact(w),
    "Move Low G (Arcade)": lambda w: (not zero(w["tgt"])) and w["gee"] <= float(mf("minWalkingGee")) and not w["cf"] and not w["restricted"],
    "Move Low G (FPS)": lambda w: (not zero(w["tgt"])) and w["gee"] <= float(mf("minWalkingGee")) and w["cf"] and not w["restricted"],
    "Low G bound": lambda w: surface_contact(w) and (not zero(w["tgt"])) and abs(w["dh"]) < 5.0 and not w["packed"]
    and tstate(w) > float(min(mf("boundFrequency"), w["lbs"])) and not w["restricted"],
    "Low G bound land": lambda w: w["splashed"] or (abs(float(w["hfs"])) < float(HALF_HEIGHT) + float(mf("boundThreshold")) and tstate(w) > 0.10000000149011612)
    or (surface_contact(w) and tstate(w) > 0.5),
    "Low G Bound Fall": lambda w: (not surface_or_splashed(w)) and tstate(w) > float(w["lbs"]) * float(mf("boundFallThreshold")) and not w["restricted"],  # (double)(float * float): no float rounding
    "Start Run": lambda w: key(w, "Run", "held") and not w["jet"] and under_control(w) and not w["restricted"],
    "End Run": lambda w: not key(w, "Run", "held"),
    "Stop": lambda w: zero(w["tgt"]),
    "Jump Start": lambda w: key(w, "Jump", "held") and under_control(w) and not w["pm"] and not w["restricted"],
    "Landing": lambda w: surface_contact(w) and w["frames"] > 2,
    "Fall": lambda w: abs(float(w["hft"])) > float(mf("onFallHeightFromTerrain")) and not surface_or_splashed(w),
    "Feet Wet": lambda w: w["splashed"] and not surface_contact(w),
    "Feet Dry": lambda w: surface_contact(w),
    "Swim Forward": lambda w: not zero(w["tgt"]),
    "Swim Stop": lambda w: zero(w["tgt"]),
    "Flag Plant Heading Acquired": lambda w: abs(f32(w["dh"] + w["ldh"])) < 30.0,
    "Pack Toggle": lambda w: key(w, "TogglePack", "down") and under_control(w) and not w["pm"],
    "Ladder Let Go": lambda w: key(w, "Jump", "down") and under_control(w) and not w["restricted"],
    "Ladder Lean (Start)": lambda w: key(w, "Run", "held") and under_control(w) and not w["restricted"],
    "Ladder Lean (End)": lambda w: not key(w, "Run", "held"),
    "Ladder Push Off Start": lambda w: key(w, "Jump", "held") and under_control(w) and not w["restricted"],
    "Ladder Stop": lambda w: zero(w["ltr"]),
    "Seat Eject": lambda w: w["noparent"],
    "Grapple Release": lambda w: w["noparent"],
}


def ladder_dot(w):
    # Vector3.Dot(ladderTgtRPos, currentLadder.transform.up * (invLadderAxis ? -1f : 1f))
    up = axes(ROTATIONS[w["clrot"]])[0]
    return dot(w["ltr"], vmul(up, -1.0 if w["inv"] else 1.0))


PRED["Ladder Climb"] = lambda w: w["cl"] and ((not w["chk"]) or w["ccl"]) and not w["restricted"] and ladder_dot(w) > f32(0.3)
PRED["Ladder Descend"] = lambda w: w["cl"] and ((not w["chk"]) or w["cdn"]) and not w["restricted"] and ladder_dot(w) < f32(-0.3)
PRED["Ladder End"] = lambda w: (not w["cl"]) or w["ntrig"] == 0 or (not w["lad_active"]) or (not w["lad_enabled"])
PRED["Ladder End Reached"] = lambda w: w["cl"] and w["chk"] and w["spd"] == 0.0 and ((not w["ccl"]) or (not w["cdn"]))
PRED["Clamber Reach P1"] = lambda w: w["cpath"] and w["cnt"] >= f32(0.2)
PRED["Clamber Reach P2"] = lambda w: w["cpath"] and w["cnt"] >= f32(0.3)
PRED["Clamber Reach P3"] = lambda w: w["cpath"] and (w["cnt"] >= w["cend"] or tstate(w) > float(w["clen"]) * 0.5)
PRED["Construction Mode Entered"] = lambda w: bool((w["inconstr"] and w["weldfx"] and not w["weldmesh"]) or (w["inconstr"] and not w["weldfx"]))


def make_world(rng):
    gees = [1.0, 0.5, 0.17, 0.1700000017881393, 0.3, 0.0, 2.5, 0.169, 0.171]
    ts = [0.0, 0.05, 0.1, 0.1000000015, 0.2, 0.25, 0.3, 0.5, 0.6, 1.0, 2.0, 5.0]
    w = {}
    w["tgt"] = rng.pick([F3(v) for v in ((0.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, 0.0, 0.0), (0.0, 0.0, 3e-6))] + [rnd_vec(rng)])
    w["dh"] = f32(rng.pick([0.0, 4.9, 5.0, 29.9, 30.0, 59.9, 60.0, 61.0, -61.0, -29.0, 100.0, -4.0, f32(rng.uni(-120, 120))]))
    w["ldh"] = f32(rng.pick([0.0, 10.0, -10.0, 20.0, -25.0, 29.0]))
    w["gee"] = rng.pick(gees)
    w["cf"] = bool(rng.next() & 1)
    w["landed"] = bool(rng.next() & 1)
    w["splashed"] = bool(rng.next() % 3 == 0)
    w["packed"] = bool(rng.next() % 7 == 0)
    w["active"] = bool(rng.next() % 5 != 0)
    w["hft"] = f32(rng.pick([-1.0, 0.0, 0.3, 0.35, 0.36, 1.0, 5.0, -0.5, 100.0]))
    w["hfs"] = f32(rng.pick([0.0, 0.1, 0.2, 0.2399, 0.249, 0.25, 0.269, 0.5, -0.1, 1.0]))
    w["t"] = f32(rng.pick(ts))
    w["frames"] = rng.pick([0, 1, 2, 3, 4, 10])
    w["lbs"] = f32(rng.pick([0.05, 0.1, 0.3, 0.5, 0.7, 1.0, 3.0]))
    w["jet"] = bool(rng.next() % 4 == 0)
    w["pm"] = bool(rng.next() % 6 == 0)
    w["restricted"] = False
    keys = {}
    for kname in ("Run", "Jump", "Use", "Board", "TogglePack", "Left", "Right"):
        keys[kname] = dict(held=bool(rng.next() & 1), down=bool(rng.next() % 3 == 0), up=bool(rng.next() % 3 == 0))
    w["keys"] = keys
    w["noparent"] = bool(rng.next() & 1)
    # ladder
    w["cl"] = bool(rng.next() % 4 != 0)
    w["clrot"] = rng.next() % len(ROTATIONS)
    w["inv"] = bool(rng.next() & 1)
    w["ltr"] = rng.pick([F3(v) for v in ((0.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, -1.0, 0.0), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), (0.0, 0.31, 0.0), (0.0, 0.29, 0.0))] + [rnd_vec(rng)])
    w["chk"] = bool(rng.next() & 1)
    w["ccl"] = bool(rng.next() & 1)
    w["cdn"] = bool(rng.next() & 1)
    w["spd"] = f32(rng.pick([0.0, 0.0, 0.6, -0.6]))
    w["ntrig"] = rng.pick([0, 1, 2, 3])
    w["lad_active"] = bool(rng.next() % 5 != 0)
    w["lad_enabled"] = bool(rng.next() % 5 != 0)
    # clamber
    w["cpath"] = bool(rng.next() % 4 != 0)
    w["cnt"] = f32(rng.pick([0.0, 0.19, 0.2, 0.25, 0.3, 0.31, 0.5, 0.99, 1.0, 1.2]))
    w["cend"] = f32(rng.pick([1.0, 0.9, 0.5]))
    w["clen"] = f32(rng.pick([1.0, 2.0, 4.0]))
    w["inconstr"] = bool(rng.next() % 3 == 0)
    w["weldfx"] = bool(rng.next() & 1)
    w["weldmesh"] = bool(rng.next() & 1)
    return w


def wire(w):
    """the JSON form of a world (vectors / floats as bit patterns)"""
    o = dict(w)
    for k in ("tgt", "ltr"):
        o[k] = b3(w[k])
    for k in ("dh", "ldh", "hft", "hfs", "t", "lbs", "spd", "cnt", "cend", "clen"):
        o[k] = bits(w[k])
    return o


def conditions_family():
    rng = LCG(32061)
    cases = []
    for name, pred in PRED.items():
        for _ in range(80):
            w = make_world(rng)
            cases.append(dict(event=name, world=wire(w), expect=bool(pred(w))))
    return cases


# ---- OnEvent target decisions (the OnEvent lambdas that choose GoToStateOnEvent at run time) ---------------------------------------------------------
def land_start_target(w):
    # :3873
    st = f32(mf("stumbleThreshold"))
    if not (sqr_magnitude(w["vrb"]) > st * st) and dot(w["up"], w["lcd"]) <= f32(0.9):
        if not (abs(dot(w["fup"], w["lcn"])) > f32(0.4)) and (w["ut"] - w["lct"]) <= 0.4:
            return "Idle (Floating)"
        elif dot(w["up"], w["fup"]) > f32(0.8):
            return "Landing"
        else:
            return "Recover"
    return "Ragdoll"


def feet_wet_target(w):
    st = f32(mf("stumbleThreshold"))
    if sqr_magnitude(w["rbv"]) < st * st:
        if dot(w["up"], w["fup"]) > f32(0.75):
            return "Swim (Idle)"
        return "Recover"
    return "Ragdoll"


def feet_dry_target(w):
    if dot(w["up"], w["fup"]) > f32(0.3):
        return "Idle (Floating)"
    return "Recover"


def recover_complete_target(w):
    if w["landed"]:
        return "Idle (Grounded)"
    if w["splashed"]:
        return "Swim (Idle)"
    return "Idle (Floating)"


def start_run_target(w):
    # :3841 (GoToStateOnEvent keeps its previous value when the gee is low)
    if w["gee"] > float(mf("minWalkingGee")):
        return "Run (FPS)" if w["cf"] else "Run (Arcade)"
    return w["prev"]


def end_run_target(w):
    return "Walk (FPS)" if w["cf"] else "Walk (Arcade)"


def bound_land_target(w):
    return "Low G Bound (Grounded - FPS)" if w["cf"] else "Low G Bound (Grounded - Arcade)"


SPEEDS = [f32(x) for x in (0.0, 2.0, 3.0, 3.5, 3.5000002, 3.4999998, 5.0, 10.0)]


def decisions_family():
    rng = LCG(32071)
    out = []
    for _ in range(300):
        rot = rng.next() % len(ROTATIONS)
        up = axes(ROTATIONS[rot])[0]
        fup = normalize(rng.pick([(0.0, 1.0, 0.0), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0), (0.0, -1.0, 0.0), rnd_vec(rng)]))
        if fup == (0.0, 0.0, 0.0):
            fup = (0.0, 1.0, 0.0)
        w = dict(rot=rot, up=up, fup=fup, lcd=normalize(rnd_vec(rng)), lcn=normalize(rnd_vec(rng)),
                 vrb=(rng.pick(SPEEDS), 0.0, 0.0), rbv=(0.0, rng.pick(SPEEDS), 0.0),
                 ut=1000.0, landed=bool(rng.next() & 1), splashed=bool(rng.next() & 1),
                 gee=rng.pick([1.0, 0.5, 0.17, 0.1700000017881393, 0.169, 0.171]), cf=bool(rng.next() & 1), prev="Walk (FPS)")
        w["lct"] = 1000.0 - rng.pick([0.0, 0.1, 0.4, 0.4000001, 0.5, 2.0])
        out.append(dict(rot=rot, fup=b3(fup), lcd=b3(w["lcd"]), lcn=b3(w["lcn"]), vrb=b3(w["vrb"]), rbv=b3(w["rbv"]), ut=w["ut"], lct=w["lct"], landed=w["landed"],
                        splashed=w["splashed"], gee=w["gee"], cf=w["cf"],
                        landStart=land_start_target(w), feetWet=feet_wet_target(w), feetDry=feet_dry_target(w), recoverComplete=recover_complete_target(w),
                        startRun=start_run_target(w), endRun=end_run_target(w), boundLand=bound_land_target(w)))
    return out
