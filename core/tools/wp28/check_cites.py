#!/usr/bin/env python3
"""Check (and with --fix repair) the C# line citations above ported functions.

A cite is a comment line starting with `// File.cs:N` or a bare `// :N` (File taken from the class of the function that follows, or the
last explicit file). The function name is taken from the next code line (`Class::Name(`). The default (exact) check: N must be a
declaration line of that name in decompiled/KSP-Decompiled/Assembly-CSharp/<File>.cs (candidates(): the line where the member name
appears with its signature, not a brace or body line); a miss prints `off by k` and the right line. `--window K` keeps the legacy
check (the name appears within +-K lines of N; the old default was 2). --fix moves N to the nearest declaration line of that name.

Usage: python -I tools/wp28/check_cites.py [--fix] [--window K] [--cs-root DIR] file.cpp ...   (exit 1 on mismatches)
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
DEFAULT_CS = os.path.join(ROOT, "decompiled", "KSP-Decompiled", "Assembly-CSharp")
if not os.path.isdir(DEFAULT_CS):  # worktree: the spec lives in the main checkout
    DEFAULT_CS = r"C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\Assembly-CSharp"

CLASS_FILE = {
    "ModuleSpaceObjectDrillBase": "ModuleAsteroidDrill", "ModuleSpaceObjectInfoBase": "ModuleAsteroidInfo",
    "ModuleSpaceObjectAnalysisBase": "ModuleAsteroidAnalysis",
}
NAME_ALIAS = {
              # WP-32: C# properties / destructors / split SetupFSM
              "~Kerbal": "OnDestroy", "~CrewHatchController": "OnDestroy", "SeatCamBobEuler": "kerbalSeatCamUpdate", "set_flagItems": "flagItems", "set_currentLadderPart": "currentLadderPart", "OnALadder_": "OnALadder", "SetupFSMStates": "SetupFSM", "SetupFSMEvents": "SetupFSM", "SetupFSMEvents2": "SetupFSM", "GetCurrentColor_": "GetCurrentColor",
              "~Mission": "OnDestroy", "~TestModule": "OnDestroy", "~ActionModule": "OnDestroy", "SetTitle": "title", "SetTitleNoGui": "title",
              "ActionsContain": "ActionsContainModule", "EqualsObject": "Equals", "craftFileExists": "GetMissionCraftByFileName",
              "UpdateMissionFlowUIDirect": "UpdateMissionFlowUI", "ModulesAreEqualImpl": "ModulesAreEqual",
              "set_currentMassVal": "currentMassVal", "set_massThresholdVal": "massThresholdVal", "set_IsDraining": "IsDraining",
              "set_SemiMajorAxis": "SemiMajorAxis", "set_Eccentricity": "Eccentricity", "set_Inclination": "Inclination", "set_Lan": "Lan",
              "set_ArgumentOfPeriapsis": "ArgumentOfPeriapsis", "set_Epoch": "Epoch", "set_MeanAnomalyAtEpoch": "MeanAnomalyAtEpoch", "set_GAPGizmoIcon": "GAPGizmoIcon",
              "set_Kerbal": "Kerbal", "set_inactive": "inactive", "set_rosterStatus": "rosterStatus", "set_type": "type",
              "SetExtraExperience": "ExtraExperience", "CrewGenerator_GetLastName": "GetLastName",
              "CrewGenerator_RemoveLastName": "RemoveLastName",
              "TransferStep": "transferResources",
              "ApplyJointBreakForce": "SetJoint", "SiteRenameAcceptPressed": "RenameSite", "SiteRenameDismissPressed": "RenameSite",
              "SiteRenameAcceptEnabled": "RenameSite",
              "UpdatePlacement": "OnUpdate", "SetCurrentCargoPart": "CurrentCargoPart", "SetupPaw": "Setup", "ModuleFor": "kerbalModule",
              "WeightBodies": "GetWeightBodies", "min": "IntegerToGreek",
              # WP-53: C# properties ported as getter / Set* functions, and one renamed private function
              "UpdateGroundModuleUI": "UpdateModuleUI", "SetPowerUnitsProduced": "PowerUnitsProduced", "SetEnabled": "Enabled",
              "SetScienceLimit": "ScienceLimit", "OnDestroyed": "OnDestroy",
              # WP-29b: C# camelCase members of the wheel modules ported with an upper-case first letter; RemoveAdjuster = OnModuleAdjusterRemoved
              "WheelSetup": "wheelSetup", "OnPartPack": "onPartPack", "OnPartUnpack": "onPartUnpack", "OnDockingComplete": "onDockingComplete",
              "OnVesselUndocking": "onVesselUndocking", "ToggleRbConstraints": "toggleRbConstraints", "HoldWheelDamage": "holdWheelDamage",
              "UpdateDriftFix": "updateDriftFix", "GetFixFwd": "getFixFwd", "GetFixTorque": "getFixTorque", "RemoveAdjuster": "OnModuleAdjusterRemoved",
              "OnVesselPartCountChanged": "onVesselPartCountChanged", "UpdateCoordFrame": "updateCoordFrame", "UpdateSteering": "updateSteering",
              "FindCoMfwdLength": "findCoMfwdLength", "FindTurnCenter": "findTurnCenter",
              # WP-20b: the reduced rigid-body binding names its ports after what they compute
              "StartPartRigidbodies": "Start", "PartRigidbodyMass": "UpdateMassStats", "PartInertiaTensor": "ValidateInertiaTensor",
              "ReadState": "CalculatePhysicsStats", "MOIFromRigidbodies": "CalculatePhysicsStats", "IntegratePartBuffers": "Integrate",
              "ResumePartVelocity": "ResumeVelocity", "ApplyAeroForces": "ApplyAeroDrag", "ApplyEngineForces": "ThrustUpdate",
              "ApplyRcsForces": "FixedUpdate", "ApplySurfaceForces": "FixedUpdate",
              # WP-20c: the structural layer (a vessel-wide CycleAutoStrut, PartJoint.OnJointBreak acted on by the host); WP-16 Part.Start's
              # staging block
              "CycleAutoStruts": "CycleAutoStrut", "HandleBreak": "OnJointBreak", "StartStaging": "Start",
              # WP-14 helpers factored out of a C# function (cited at that function)
              "UpdateSurfaceSpeeds": "UpdatePosVel", "ApplyFlightIntegratorAtmosphere": "FixedUpdate"}
CITE = re.compile(r"^(\s*)// (?:([A-Za-z0-9_/]+)\.cs)?:(\d+)\b")
FUNC = re.compile(r"(\w+)::(~?\w+)\s*\(")


def load(cs_root, f, cache={}):
    key = (cs_root, f)
    if key not in cache:
        p = os.path.join(cs_root, f + ".cs")
        if "/" in f and not os.path.exists(p):  # WP-37: `Templates/PlantFlag.cs` disambiguates same-named files in different folders
            suffix = os.sep + f.replace("/", os.sep) + ".cs"
            for dp, _dn, fn in os.walk(cs_root):
                cand = os.path.join(dp, f.split("/")[-1] + ".cs")
                if f.split("/")[-1] + ".cs" in fn and cand.endswith(suffix):
                    p = cand
                    break
        if not os.path.exists(p):  # WP-31: classes in sub folders (Experience/Effects/X.cs ...): first file of that name below cs_root
            for dp, _dn, fn in os.walk(cs_root):
                if f + ".cs" in fn:
                    p = os.path.join(dp, f + ".cs")
                    break
        if not os.path.exists(p):  # WP-50: Assembly-CSharp-firstpass is a sibling assembly (SpaceNavigator*.cs, QuaternionExtensions.cs)
            fp = os.path.join(os.path.dirname(cs_root.rstrip("/\\")), "Assembly-CSharp-firstpass", f + ".cs")
            if os.path.exists(fp):
                p = fp
        cache[key] = open(p, encoding="utf-8-sig", errors="replace").read().splitlines() if os.path.exists(p) else None
    return cache[key]


def candidates(lines, name):
    decl = re.compile(r"\b" + re.escape(name) + r"\b\s*(\(|=>|$|\{)")
    out = [i + 1 for i, l in enumerate(lines) if decl.search(l) and not l.strip().startswith(("return", "//")) and not l.rstrip().endswith(";") or
           (re.search(r"\b" + re.escape(name) + r"\s*\(", l) and re.match(r"\s*(public|protected|private|internal|static|override|virtual|new)", l))]
    if not out:
        out = [i + 1 for i, l in enumerate(lines) if re.search(r"\b" + re.escape(name) + r"\b", l)]
    return sorted(set(out))


def process(path, cs_root, fix, window=None):
    src = open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    lines = src.split(nl)
    last_file = None
    bad = 0
    total = 0
    for i, l in enumerate(lines):
        m = CITE.match(l)
        if not m:
            continue
        fname = m.group(2)
        n = int(m.group(3))
        if fname:
            last_file = fname
        # the next non-comment, non-empty line names the function
        j = i + 1
        while j < len(lines) and (lines[j].strip().startswith("//") or not lines[j].strip()):
            j += 1
        fm = FUNC.search(lines[j]) if j < len(lines) else None
        if not fm:
            continue
        cls, name = fm.group(1), NAME_ALIAS.get(fm.group(2), fm.group(2))
        f = fname or CLASS_FILE.get(cls) or (cls if load(cs_root, cls) else last_file)
        if cls in ("ResourceProcessingEnvironment", "ModuleSpaceObjectDrillBase") and not fname:
            f = CLASS_FILE.get(cls)
        if cls == "ModuleAsteroidDrill" or cls == "ModuleCometDrill":
            f = cls
        cs = load(cs_root, f) if f else None
        if cs is None:
            print("%s:%d: no C# file for %s::%s" % (path, i + 1, cls, name))
            bad += 1
            total += 1
            continue
        total += 1
        if window is None:  # exact: N must itself be a declaration line
            ok = n in candidates(cs, name)
        else:
            ok = any(re.search(r"" + re.escape(name) + r"", cs[k - 1]) for k in range(max(1, n - window), min(len(cs), n + window) + 1))
        if ok and fname:
            continue
        if ok:  # right line, but the file name is missing
            if fix:
                lines[i] = CITE.sub(lambda mm: "%s// %s.cs:%s" % (mm.group(1), f, mm.group(3)), l, count=1)
            else:
                print("%s:%d: bare cite :%d (add %s.cs)" % (path, i + 1, n, f))
                bad += 1
            continue
        cands = candidates(cs, name)
        if not cands:
            print("%s:%d: %s.cs has no %s (cite :%d)" % (path, i + 1, f, name, n))
            bad += 1
            continue
        best = min(cands, key=lambda c: abs(c - n))
        print("%s:%d: %s.cs:%d -> %d (%s) off by %d" % (path, i + 1, f, n, best, name, abs(best - n)))
        if fix:
            new = (CITE.sub(lambda mm: "%s// %s:%d" % (mm.group(1), (mm.group(2) + ".cs") if mm.group(2) else (f + ".cs"), best), l, count=1))
            lines[i] = new
        else:
            bad += 1
    if fix:
        open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
    return bad, total


def main():
    args = sys.argv[1:]
    fix = "--fix" in args
    cs = DEFAULT_CS
    if "--cs-root" in args:
        k = args.index("--cs-root")
        cs = args[k + 1]
        del args[k:k + 2]
    window = None
    if "--window" in args:
        k = args.index("--window")
        window = int(args[k + 1])
        del args[k:k + 2]
    files = [a for a in args if not a.startswith("--")]
    bad = total = 0
    for f in files:
        b, t = process(f, cs, fix, window)
        bad += b
        total += t
    print("%d cites checked, %d mismatches" % (total, bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
