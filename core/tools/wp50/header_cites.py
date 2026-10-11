#!/usr/bin/env python3
"""Checks (and with --fix repairs) the `// :N` C# line cites on the member declarations of include/ksp/input/*.hpp.

The .cpp cites are checked by tools/wp28/check_cites.py; this does the same for the header declarations: the member name is the
identifier before `(` (or the declared variable), the C# file comes from the enclosing class, N must be a declaration line of that
name in the C# file. Overloaded constructors / methods listed in declaration order are matched to the C# declarations in order.
Usage: python -I tools/wp50/header_cites.py [--fix] <core dir>
"""
import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("check_cites", os.path.join(HERE, "..", "wp28", "check_cites.py"))
cc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cc)

CLASS_FILE = {
    "KeyCodeExtended": "KeyCodeExtended", "InputBinding": "InputBinding", "KeyBinding": "KeyBinding", "InputDevices": "InputDevices",
    "AxisBinding_Single": "AxisBinding_Single", "AxisBinding": "AxisBinding", "AxisKeyBinding": "AxisKeyBinding",
    "AxisKeyBindingList": "AxisKeyBindingList", "InputLockManager": "InputLockManager", "ExtendedInput": "ExtendedInput",
    "FlightInputHandler": "FlightInputHandler", "AxisGroupsInputHandler": "AxisGroupsInputHandler", "KeyboardLayout": "KeyboardLayout",
    "TrackIRSettings": "TrackIR", "SpaceNavigator": "SpaceNavigator", "SpaceNavigatorNoDevice": "SpaceNavigatorNoDevice",
    "SpaceNavigatorWindows": "SpaceNavigatorWindows", "Mouse": "Mouse", "MouseButton": "Mouse", "InputSettings": "InputSettings",
    "CanvasGroupInputLock": "CanvasGroupInputLock", "KSPGraphicRaycaster": "KSPGraphicRaycaster",
    "DialogMouseEnterControlLock": "DialogMouseEnterControlLock", "UIInputExtraMouseButtons": "UIInputExtraMouseButtons",
}
FILE_FOR_NAMESPACE = {"GameSettings": "GameSettings"}
NAME_ALIAS = {"QuaternionPitch": ("QuaternionExtensions", "Pitch"), "QuaternionYaw": ("QuaternionExtensions", "Yaw"),
              "QuaternionRoll": ("QuaternionExtensions", "Roll"), "ToBits": (None, "ToBits")}
ALIAS_NAME = {"name": "name", "isNone": "isNone", "deadzone": "deadzone", "scale": "scale", "SetInstance": "Instance", "Translation": "Translation",
              "Rotation": "Rotation", "Available": "Available", "LockMask": "LockMask", "SetLockMask": "LockMask", "Left": "Left",
              "Right": "Right", "Middle": "Middle", "screenPos": "screenPos", "RCSLock": "RCSLock", "Length": "Length",
              "Node": "Node", "InputLockMask": "InputLockMask"}
CITE = re.compile(r"//\s*(?:([A-Za-z0-9_]+)\.cs)?:(\d+)\b")


def member_name(code):
    m = re.search(r"(~?\w+)\s*\(", code)
    if m and not code.strip().startswith(("static inline", "inline", "using", "if", "return")):
        return m.group(1)
    m = re.search(r"(\w+)\s*(?:=[^;]*)?;", code)
    return m.group(1) if m else None


def main():
    fix = "--fix" in sys.argv
    core = [a for a in sys.argv[1:] if not a.startswith("--")][0]
    root = os.path.join(core, "include", "ksp", "input")
    bad = total = 0
    for fn in sorted(os.listdir(root)):
        if not fn.endswith(".hpp"):
            continue
        path = os.path.join(root, fn)
        text = open(path, encoding="utf-8", newline="").read()
        nl = "\r\n" if "\r\n" in text else "\n"
        lines = text.split(nl)
        cls = None
        ns = None
        seen = {}
        for i, l in enumerate(lines):
            mc = re.match(r"^\s*(?:class|struct)\s+(\w+)[^;]*$", l)
            if mc and "{" in l or (mc and not l.rstrip().endswith(";")):
                cls = mc.group(1)
            mn = re.match(r"^namespace (\w+) \{", l)
            if mn:
                ns = mn.group(1)
            m = CITE.search(l)
            if not m or l.strip().startswith("//"):
                continue
            code = l[:m.start()]
            name = member_name(code)
            if name is None:
                continue
            f = cc.load(cc.DEFAULT_CS, CLASS_FILE.get(cls, cls)) if cls else None
            fname = CLASS_FILE.get(cls, cls) if cls else None
            if ns in FILE_FOR_NAMESPACE and (cls is None or cls not in CLASS_FILE):
                fname = FILE_FOR_NAMESPACE[ns]
            if name in NAME_ALIAS:
                alias = NAME_ALIAS[name]
                fname = alias[0] or fname
                name = alias[1]
            if name == cls:  # constructors
                pass
            cs = cc.load(cc.DEFAULT_CS, fname) if fname else None
            if cs is None:
                fp = os.path.join(os.path.dirname(cc.DEFAULT_CS), "Assembly-CSharp-firstpass", (fname or "") + ".cs")
                cs = open(fp, encoding="utf-8-sig").read().splitlines() if os.path.exists(fp) else None
            if cs is None:
                continue
            cname = name
            if name == "SetInstance":
                cname = "Instance"
            elif name == "SetLockMask":
                cname = "LockMask"
            elif cname.endswith("_"):
                cname = cname.rstrip("_")
                if cname == "deviceAvailable":
                    cname = "_deviceAvailable"
                elif cname.startswith("fake"):
                    cname = "_" + cname
            cands = cc.candidates(cs, cname)
            total += 1
            key = (fname, cname)
            idx = seen.get(key, 0)
            seen[key] = idx + 1
            n = int(m.group(2))
            if not cands:
                print("%s:%d: no %s in %s.cs (cite :%d)" % (fn, i + 1, cname, fname, n))
                bad += 1
                continue
            if n in cands:
                continue
            # overload series: take the idx-th declaration when the counts look like a series
            target = cands[idx] if idx < len(cands) and len(cands) > 2 else min(cands, key=lambda c: abs(c - n))
            print("%s:%d: %s.cs:%d -> %d (%s)" % (fn, i + 1, fname, n, target, cname))
            bad += 1
            if fix:
                lines[i] = l[:m.start()] + "// :%d" % target + l[m.end():] if not m.group(1) else l[:m.start()] + "// %s.cs:%d" % (m.group(1), target) + l[m.end():]
        if fix:
            open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
    print("%d cites checked, %d off" % (total, bad))
    return 1 if bad and not fix else 0


sys.exit(main())
