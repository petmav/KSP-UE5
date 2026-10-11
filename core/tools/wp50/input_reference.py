#!/usr/bin/env python3
"""WP-50 independent reference (no kspcore imports) for the input layer.

Writes core/tests/data/wp50_reference.json. Everything below follows Assembly-CSharp/{ControlTypes,InputLockManager,KeyCodeExtended,
InputBinding,KeyBinding,AxisBinding_Single,AxisBinding,FlightInputHandler,GameSettings,Mouse}.cs and
UnityEngine.CoreModule/KeyCode.cs written out in plain Python (struct float32 stores, Mono's rule for managed float arithmetic: a float
expression is evaluated in double and stored as float at each assignment / argument / return, math.* for the C runtime functions).

Inputs: the decompiled C# text (parsed here, independently of tools/wp50/gen_*.py), the shipped keyboard layouts
core/testdata/GameData-mirror/GameData/Squad/Controls/*.cfg (original game data). Randomised fixtures use a fixed seed and are
documented as FIXTURE in the output.
Run:  python -I core/tools/wp50/input_reference.py
"""
import glob
import json
import math
import os
import random
import re
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.normpath(os.path.join(HERE, "..", ".."))


def _spec_root():
    for c in (os.path.join(CORE, "..", "decompiled"), os.path.join(CORE, "..", "..", "..", "decompiled"),
              os.path.join("C:" + os.sep, "Projects", "KSP-UE5", "decompiled")):
        p = os.path.join(c, "KSP-Decompiled", "Assembly-CSharp", "ControlTypes.cs")
        if os.path.exists(p):
            return os.path.dirname(p), os.path.normpath(os.path.join(os.path.dirname(p), "..", "Unity", "UnityEngine.CoreModule", "UnityEngine"))
    raise SystemExit("decompiled spec not found")


ACS, UNITY = _spec_root()
CONTROLS = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Controls")
OUT = os.path.join(CORE, "tests", "data", "wp50_reference.json")
if not os.path.isdir(CONTROLS):  # worktree: the original game data mirror may live in the main checkout
    CONTROLS = os.path.join("C:" + os.sep, "Projects", "KSP-UE5", "core", "testdata", "GameData-mirror", "GameData", "Squad", "Controls")

M64 = (1 << 64) - 1


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def f32bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


# ---------------------------------------------------------------------------------------------- enums from the C# text
def parse_enum(path, suffix_u=False):
    out = []
    for line in open(path, encoding="utf-8-sig").read().splitlines():
        m = re.match(r"\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+|-?\d+)(?:uL|u)?,?\s*$", line)
        if m:
            out.append((m.group(1), int(m.group(2), 0)))
    return out


CONTROL_TYPES = parse_enum(os.path.join(ACS, "ControlTypes.cs"))
KEYCODES = parse_enum(os.path.join(UNITY, "KeyCode.cs"))
CT = dict(CONTROL_TYPES)
KC = dict(KEYCODES)


def enum_parse_keycode(token):
    """Enum.Parse(typeof(KeyCode), token): (kind, value) with kind in ok / argument / overflow"""
    s = token.strip()
    if not s:
        return ("argument", None)
    if s[0].isdigit() or s[0] in "-+":
        m = re.fullmatch(r"[+-]?\d+", s)
        if m:
            v = int(s)
            if v < -(1 << 31) or v >= (1 << 31):
                return ("overflow", None)
            return ("ok", v)
    acc = 0
    for part in s.split(","):
        n = part.strip()
        if n not in KC:
            return ("argument", None)
        acc |= KC[n]
    return ("ok", acc)


def keycode_tostring(v):
    names = [n for n, x in KEYCODES if x == v]
    return names[0] if names else str(v)


def kce(token):
    """KeyCodeExtended(string): (code, name, ToString); None when Enum.Parse overflows"""
    kind, v = enum_parse_keycode(token)
    if kind == "overflow":
        return None
    if kind == "ok":
        return (v, "", keycode_tostring(v))
    return (0, token, token if token else "None")


def flags_tostring_controltypes(v):
    """Enum.ToString of the [Flags] ControlTypes = Mono's Enum.InternalFlagsFormat (decompiled/.../Mono/mscorlib/System/Enum.cs:151; the
    flags path has NO exact-name shortcut). ControlTypes declares two members with value 0 (None, STAGE_IN_MAP_CONTROLS); which of them
    ends up at index 0 / 1 of the sorted table is Array.Sort's (unstable) tie order, so the two names are the tokens <Z0> / <Z1>."""
    zeros = [n for n, x in CONTROL_TYPES if x == 0]
    rest = sorted(((n, x) for n, x in CONTROL_TYPES if x != 0), key=lambda nv: nv[1])
    names = ["<Z0>", "<Z1>"] + [n for n, _x in rest]
    vals = [0, 0] + [x for _n, x in rest]
    assert len(zeros) == 2
    num = v
    res = []
    i = len(vals) - 1
    while i >= 0 and (i != 0 or vals[i] != 0):
        if (num & vals[i]) == vals[i]:
            num -= vals[i]
            res.insert(0, names[i])
        i -= 1
    if num != 0:
        return str(v)
    if v == 0:
        return names[0]
    return ", ".join(res)


# ---------------------------------------------------------------------------------------------- InputLockManager
class DotNetDict:
    """Dictionary<string, ulong> insertion / removal / enumeration order (entries array + LIFO free list)"""

    def __init__(self):
        self.entries = []
        self.free = []
        self.index = {}

    def set(self, k, v):
        if k in self.index:
            self.entries[self.index[k]][1] = v
            return
        if self.free:
            i = self.free.pop()
            self.entries[i] = [k, v]
        else:
            i = len(self.entries)
            self.entries.append([k, v])
        self.index[k] = i

    def remove(self, k):
        if k not in self.index:
            return
        i = self.index.pop(k)
        self.entries[i] = None
        self.free.append(i)

    def clear(self):
        self.entries, self.free, self.index = [], [], {}

    def items(self):
        return [tuple(e) for e in self.entries if e is not None]


GUI = CT["GUI"]
ALL = CT["All"]


def lock_sequence(seed, count):
    rng = random.Random(seed)
    ids = ["lockA", "lockB", "lockC", "tweakableMenu", "TimeWarpLock", "editor", "dialog", "manualStageLock", "KSCpopup", "flagDlg"]
    flags = [n for n, v in CONTROL_TYPES if v and (v & (v - 1)) == 0]  # single-bit members
    ops = []
    for _ in range(count):
        r = rng.random()
        if r < 0.55:
            k = rng.choice(ids)
            if rng.random() < 0.15:
                mask = ALL
            else:
                mask = 0
                for _ in range(rng.randint(0, 6)):
                    mask |= CT[rng.choice(flags)]
                if rng.random() < 0.3:
                    mask |= GUI
            ops.append({"op": "set", "id": k, "mask": str(mask)})
        elif r < 0.9:
            ops.append({"op": "remove", "id": rng.choice(ids)})
        elif r < 0.95:
            ops.append({"op": "clear"})
        else:
            ops.append({"op": "setall", "id": rng.choice(ids)})
    return ops


def run_locks(ops):
    d = DotNetDict()
    mask = 0
    results = []
    probes = [CT["PITCH"], CT["THROTTLE"], GUI, CT["STAGING"] | CT["SAS"], CT["ALL_SHIP_CONTROLS"], CT["EDITOR_LOCK"], ALL]
    for o in ops:
        events = []
        if o["op"] in ("set", "setall"):
            old = mask
            m = int(o["mask"]) if o["op"] == "set" else ALL
            d.set(o["id"], m)
            mask = 0
            for _k, v in d.items():
                mask |= v
            events.append(["mod", str(old), str(mask)])
            if (old & GUI) != 0 and (mask & GUI) == 0:
                events.append(["guilock"])
                events.append(["tooltip"])
        elif o["op"] == "remove":
            if o["id"] in d.index:
                old = mask
                d.remove(o["id"])
                mask = 0
                for _k, v in d.items():
                    mask |= v
                events.append(["mod", str(old), str(mask)])
                if (old & GUI) == 0 and (mask & GUI) != 0:
                    events.append(["guiunlock"])
        elif o["op"] == "clear":
            old = mask
            d.clear()
            mask = 0
            events.append(["mod", str(old), str(mask)])
        q = {}
        for p in probes:
            q[str(p)] = [int(mask & p > 0), int((mask & p) == 0), int((mask & p) == p), int((mask & p) != p)]
        stack = d.items()
        bits = bin(mask)[2:] if mask else "0"
        results.append({"events": events, "mask": str(mask), "order": [k for k, _v in stack],
                        "values": [str(v) for _k, v in stack], "bits": bits, "queries": q})
    return results


def control_locks_table():
    out = []
    for lower in (-70, -64, -63, -5, -1, 0, 1, 3, 17, 40, 62, 63, 64, 70):
        for upper in (-3, 0, 1, 2, 5, 31, 62, 63, 64, 100):
            if lower > upper:
                v = 0
            else:
                num = upper - lower + 1
                if num >= 64:
                    v = 0
                else:
                    num2 = ((1 << num) - 1) & M64
                    shl = lower & 63
                    shr = (-lower) & 63
                    v = (((num2 << shl) & M64) | (num2 >> shr)) & M64
            out.append([lower, upper, str(v)])
    return out


# ---------------------------------------------------------------------------------------------- cfg layouts
class Node:
    def __init__(self, name=""):
        self.name = name
        self.values = []  # [name, value]
        self.nodes = []


def parse_cfg(text):
    root = Node("")
    stack = [root]
    pending = None
    for raw in text.replace("\r", "").split("\n"):
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if line == "{":
            n = Node(pending)
            stack[-1].nodes.append(n)
            stack.append(n)
            pending = None
        elif line == "}":
            stack.pop()
        elif "=" in line:
            i = line.index("=")
            stack[-1].values.append([line[:i].strip(), line[i + 1:].strip()])
        else:
            pending = line
    return root


def copy_to(src, dst, overwrite):
    """ConfigNode.CopyToRecursive"""
    if dst.name == "":
        dst.name = src.name
    for k, v in src.values:
        if overwrite:
            for e in dst.values:
                if e[0] == k:
                    e[1] = v
                    break
            else:
                dst.values.append([k, v])
        else:
            dst.values.append([k, v])
    for c in src.nodes:
        if overwrite:
            for i in range(len(dst.nodes) - 1, -1, -1):
                if dst.nodes[i].name == c.name:
                    del dst.nodes[i]
                    break
        n = Node(c.name)
        dst.nodes.append(n)
        copy_to(c, n, False)


def deep_copy(n):
    r = Node(n.name)
    copy_to(n, r, False)
    return r


def find(n, name):
    for c in n.nodes:
        if c.name == name:
            return c
    return None


def parse_layout(files, loaded, path, node, depth=0):
    """GameSettings.ParseLayoutRecursive; `files` maps file name -> text, `loaded` the layoutConfigNodes dictionary"""
    base = os.path.basename(path)
    if base not in files:
        return node
    cfg = parse_cfg(files[base])
    kl = find(cfg, "KEYBOARD_LAYOUT")
    if kl is None:
        return node
    dep = [v for k, v in kl.values if k == "dependency"]
    if dep and depth < 5:
        depth += 1
        node = deep_copy(loaded[dep[0]]) if dep[0] in loaded else parse_layout(files, loaded, dep[0], node, depth)
    if find(cfg, "KEY_MAP") is not None and find(node, "KEY_MAP") is not None:
        copy_to(find(cfg, "KEY_MAP"), find(node, "KEY_MAP"), True)
        for i in range(len(cfg.nodes) - 1, -1, -1):
            if cfg.nodes[i].name == "KEY_MAP":
                del cfg.nodes[i]
                break
    copy_to(cfg, node, True)
    return node


def norm_binding(node):
    d = dict((k, v) for k, v in node.values)
    prim = kce(d["primary"]) if "primary" in d else None
    sec = kce(d["secondary"]) if "secondary" in d else None
    return [prim[2], sec[2], str(int(d["group"])), str(int(d["modeMask"])), str(int(d["modeMaskSec"]))]


def layouts():
    names = sorted((os.path.basename(p) for p in glob.glob(os.path.join(CONTROLS, "*.cfg"))), key=lambda s: s.upper())
    files = {n: open(os.path.join(CONTROLS, n), encoding="utf-8-sig").read() for n in names}
    loaded = {}
    out = {"order": names, "layouts": {}}
    for n in names:
        merged = parse_layout(files, loaded, n, Node(""))
        loaded[n] = merged
        b = {}
        for c in merged.nodes:
            if c.name in ("KEYBOARD_LAYOUT", "KEY_MAP"):
                continue
            b[c.name] = norm_binding(c)
        km = find(merged, "KEY_MAP")
        out["layouts"][n] = {"bindings": b, "keymap_count": len(km.values) if km else 0,
                             "layout_name": [v for k, v in find(merged, "KEYBOARD_LAYOUT").values if k == "layout"]}
    return out


# ---------------------------------------------------------------------------------------------- KeyBinding ctor table
def keybinding_default(expr):
    """new KeyBinding(args): primary, secondary, switchState, switchStateSecondary, group (the C# overload set)"""
    args = [a.strip() for a in re.match(r"new KeyBinding\((.*)\)$", expr).group(1).split(",") if a.strip()]
    kinds = []
    for a in args:
        if a.startswith("KeyCode."):
            kinds.append("K")
        elif a.startswith("InputBindingModes."):
            kinds.append("M")
        elif a.startswith("ControlTypes."):
            kinds.append("C")
        else:
            raise SystemExit("unhandled KeyBinding arg " + a)
    sig = "".join(kinds)
    val = [a.split(".")[1] for a in args]
    modes = dict(parse_enum_names(os.path.join(ACS, "InputBindingModes.cs")))
    r = {"primary": "None", "secondary": "None", "ss": -1, "ssec": -1, "group": 0}
    if sig == "":
        pass
    elif sig == "C":
        r["group"] = CT[val[0]]
    elif sig == "K":
        r["primary"] = val[0]
    elif sig == "KC":
        r["primary"] = val[0]
        r["group"] = CT[val[1]]
    elif sig == "KM":
        r["primary"] = val[0]
        r["ss"] = modes[val[1]]
    elif sig == "KMC":
        r["primary"] = val[0]
        r["ss"] = modes[val[1]]
        r["group"] = CT[val[2]]
    elif sig == "KK":
        r["primary"], r["secondary"] = val
    elif sig == "KKC":
        r["primary"], r["secondary"] = val[0], val[1]
        r["group"] = CT[val[2]]
    elif sig == "KKMM":
        r["primary"], r["secondary"] = val[0], val[1]
        r["ss"], r["ssec"] = modes[val[2]], modes[val[3]]
    elif sig == "KKMMC":
        r["primary"], r["secondary"] = val[0], val[1]
        r["ss"], r["ssec"] = modes[val[2]], modes[val[3]]
        r["group"] = CT[val[4]]
    else:
        raise SystemExit("unhandled KeyBinding overload " + sig)
    return [r["primary"], r["secondary"], str(r["group"]), str(r["ss"]), str(r["ssec"])]


def parse_enum_names(path):
    out = []
    for line in open(path, encoding="utf-8-sig").read().splitlines():
        m = re.match(r"\s*(\w+)\s*=\s*(-?\d+),?\s*$", line)
        if m:
            out.append((m.group(1), int(m.group(2))))
    return out


# ---------------------------------------------------------------------------------------------- GameSettings.SetDefaultValues
GS = open(os.path.join(ACS, "GameSettings.cs"), encoding="utf-8-sig").read().splitlines()


def gs_fields():
    f = {}
    for line in GS:
        m = re.match(r"^\t(public|internal|private) static ([\w<>\.]+) (\w+)(?: = (.+))?;$", line)
        if m:
            f[m.group(3)] = (m.group(1), m.group(2))
    return f


def default_values():
    """every `NAME = literal;` of SetDefaultValues evaluated: ints, bools, strings, floats (float32), doubles; and the KeyBinding news"""
    fields = gs_fields()
    start = next(i for i, l in enumerate(GS) if "public static void SetDefaultValues()" in l)
    scalars, keybinds, axes = {}, {}, {}
    i = start + 2
    while GS[i].strip() != "}":
        line = GS[i].strip()
        i += 1
        m = re.match(r"^(\w+) = (.+);$", line)
        name, expr = m.groups()
        ty = fields[name][1]
        if fields[name][0] != "public":
            continue  # internal / private (not in settings.cfg, not in the reflection table)
        if expr.startswith("new KeyBinding("):
            keybinds[name] = keybinding_default(expr)
        elif expr.startswith("new "):
            axes[name] = expr
        elif ty == "bool":
            scalars[name] = ["bool", expr == "true"]
        elif ty == "int":
            scalars[name] = ["int", int(expr)]
        elif ty == "string":
            scalars[name] = ["string", json.loads(expr)]
        elif ty == "float":
            scalars[name] = ["float", f32bits(float(expr.rstrip("f")))]
        elif ty == "double":
            scalars[name] = ["double", float(expr)]
        else:
            scalars[name] = ["enum", expr]
    return {"scalars": scalars, "keybindings": keybinds, "axes": axes}


def fmt_g(v, digits):
    s = format(v, ".%dg" % digits)
    if "e" in s:
        mant, exp = s.split("e")
        s = mant + "E" + exp[0] + exp[1:].lstrip("0").rjust(2, "0")
    return s


def default_text():
    """Convert.ToString(value, InvariantCulture) of every scalar default (Mono: float G7, double G15)"""
    d = default_values()["scalars"]
    out = {}
    for k, (ty, v) in d.items():
        if ty == "bool":
            out[k] = "True" if v else "False"
        elif ty == "int":
            out[k] = str(v)
        elif ty == "string":
            out[k] = v
        elif ty == "float":
            out[k] = fmt_g(struct.unpack("<f", struct.pack("<I", v))[0], 7)
        elif ty == "double":
            out[k] = fmt_g(v, 15)
    return out


# ---------------------------------------------------------------------------------------------- axis maths
def clamp01(v):
    return 0.0 if v < 0.0 else (1.0 if v > 1.0 else v)


def axis_single(raw, scale, inverted, preinvert, deadzone, sens, neutral, tag_none=False, switch_ok=True, locked=False):
    """AxisBinding_Single.GetAxis (all float inputs are float32 values)"""
    if tag_none or not switch_ok or locked:
        return neutral
    v = f32(raw * scale)
    if inverted != preinvert:
        v = f32(v * -1.0)
    if abs(v) < deadzone:
        v = 0.0
    else:
        if v > 0.0:
            v = f32((v - deadzone) * (1.0 / (1.0 - deadzone)))
        if v < 0.0:
            v = f32((v + deadzone) * (1.0 / (1.0 - deadzone)))
    if v > 0.0:
        v = f32(math.pow(clamp01(v), sens))
    if v < 0.0:
        v = f32(0.0 - f32(math.pow(clamp01(abs(v)), sens)))
    return v


def axis_cases(seed, count):
    rng = random.Random(seed)
    out = []
    for _ in range(count):
        raw = f32(rng.uniform(-1.0, 1.0)) if rng.random() < 0.9 else f32(rng.choice([0.0, 1.0, -1.0, 0.05, -0.05]))
        scale = f32(rng.choice([1.0, 0.5, 2.0, 1.5, f32(rng.uniform(0.1, 3.0))]))
        dead = f32(rng.choice([0.0, 0.05, 0.1, 0.25, f32(rng.uniform(0.0, 0.5))]))
        sens = f32(rng.choice([1.0, 0.25, 2.0, 5.0, f32(rng.uniform(0.25, 5.0))]))
        inv = rng.random() < 0.3
        pre = rng.random() < 0.15
        neutral = f32(rng.choice([0.0, -1.0, 0.0, 0.5]))
        raw2 = f32(rng.uniform(-1.0, 1.0))
        out.append({"raw": raw, "scale": scale, "dead": dead, "sens": sens, "inv": inv, "pre": pre, "neutral": neutral, "raw2": raw2,
                    "expect": axis_single(raw, scale, inv, pre, dead, sens, neutral),
                    "expect2": axis_single(raw2, scale, inv, pre, dead, sens, neutral)})
    # AxisBinding.GetAxis: the larger deflection from its own neutral wins (>=, so the primary on a tie)
    for c in out:
        p, s = c["expect"], c["expect2"]
        pa = abs(f32(p - c["neutral"]))
        sa = abs(f32(s - c["neutral"]))
        c["combined"] = p if pa >= sa else s
    return out


# ---------------------------------------------------------------------------------------------- FlightInputHandler
DT = [0.02, 0.0166667, 0.033, 0.05]


def flight_reference(seed, frames):
    """FlightInputHandler.FixedUpdate (the non-EVA, no-SpaceNav path) over scripted frames. The keys are KeyCode names; the bindings are
    the shipped Qwerty layout's. Axis bindings are the raw scripted analogue values through AxisBinding_Single.GetAxis."""
    qwerty = parse_cfg(open(os.path.join(CONTROLS, "Qwerty.cfg"), encoding="utf-8-sig").read())
    kb = {}
    for c in qwerty.nodes:
        if c.name in ("KEYBOARD_LAYOUT", "KEY_MAP"):
            continue
        d = dict((k, v) for k, v in c.values)
        kb[c.name] = (d["primary"], d["secondary"], int(d["modeMask"]), int(d["modeMaskSec"]), int(d["group"]))
    rng = random.Random(seed)
    # fixtures: lock masks per phase, axis binding configuration of the 12 AXIS_*: raw name, deadzone, sens, scale, inverted
    axis_cfg = {
        "AXIS_PITCH": dict(name="axisPitch", dz=0.05, sens=1.0, scale=1.0, inv=False),
        "AXIS_ROLL": dict(name="axisRoll", dz=0.1, sens=2.0, scale=1.0, inv=True),
        "AXIS_YAW": dict(name="axisYaw", dz=0.05, sens=1.0, scale=1.5, inv=False),
        "AXIS_THROTTLE": dict(name="axisThrottle", dz=0.0, sens=1.0, scale=1.0, inv=False, neutral=-1.0),
        "AXIS_THROTTLE_INC": dict(name="axisThrottleInc", dz=0.1, sens=1.0, scale=1.0, inv=False),
        "AXIS_TRANSLATE_X": dict(name="axisX", dz=0.05, sens=1.0, scale=1.0, inv=False),
        "AXIS_TRANSLATE_Y": dict(name="axisY", dz=0.05, sens=1.0, scale=1.0, inv=True),
        "AXIS_TRANSLATE_Z": dict(name="axisZ", dz=0.05, sens=0.5, scale=1.0, inv=False),
        "AXIS_WHEEL_STEER": dict(name="axisWheelSteer", dz=0.05, sens=1.0, scale=1.0, inv=False),
        "AXIS_WHEEL_THROTTLE": dict(name="axisWheelThrottle", dz=0.05, sens=1.0, scale=1.0, inv=False),
    }
    lock_pool = ["PITCH", "ROLL", "YAW", "THROTTLE", "LINEAR", "WHEEL_STEER", "WHEEL_THROTTLE", "CUSTOM_ACTION_GROUPS"]
    keys = ["W", "S", "A", "D", "Q", "E", "LeftShift", "LeftControl", "I", "K", "J", "L", "H", "N", "LeftAlt", "CapsLock", "X", "Z"]
    st = dict(mainThrottle=0.0, roll=0.0, yaw=0.0, pitch=0.0, rollTrim=0.0, yawTrim=0.0, pitchTrim=0.0, wheelSteer=0.0, wheelSteerTrim=0.0,
              wheelThrottle=0.0, wheelThrottleTrim=0.0, X=0.0, Y=0.0, Z=0.0)
    custom = [0.0] * 4
    prec = dict(pitch=0.0, roll=0.0, yaw=0.0, X=0.0, Y=0.0, Z=0.0, ws=0.0, wt=0.0)
    prec_custom = [0.0] * 4
    throttle = 0.0
    axisThrottle = 0.0
    lastAxisThrottle = 0.0
    refocus = True
    throttleFocus = False
    precisionMode = False
    sens_kb = f32(2.0)
    out_frames = []
    inputs = []
    lock_mask = 0
    for fno in range(frames):
        if fno % 40 == 0:
            lock_mask = 0
            for n in lock_pool:
                if rng.random() < 0.15:
                    lock_mask |= CT[n]
        dt = f32(rng.choice(DT))
        pressed = [k for k in keys if rng.random() < 0.18]
        if rng.random() < 0.05:
            pressed = []
        keydown = []
        if "LeftAlt" in pressed and rng.random() < 0.3:
            keydown.append("X")
        if rng.random() < 0.1:
            precisionMode = not precisionMode
        refocus_event = rng.random() < 0.06
        if refocus_event:
            refocus = True
        axes = {}
        for a, cfg in axis_cfg.items():
            axes[cfg["name"]] = f32(rng.uniform(-1, 1)) if rng.random() < 0.6 else 0.0
        inputs.append({"dt": dt, "pressed": pressed, "keydown": keydown, "axes": axes, "lock": str(lock_mask), "precision": precisionMode,
                       "refocus": refocus_event})

        def mode_ok(switch):
            return True  # not the flight scene's FlightUIModeController in this fixture (CompareSwitchState returns true)

        def keydown_state(name):
            """KeyBinding.GetKey: locked by its group (0 here), primary / secondary pressed (switch states always compare true here)"""
            p, s, ms, mss, grp = kb[name]
            return (p in pressed) or (s in pressed)

        def locked(ct):
            return (lock_mask & CT[ct]) != 0

        def axis(name):
            cfg = axis_cfg[name]
            neutral = f32(cfg.get("neutral", 0.0))
            return axis_single(axes[cfg["name"]], f32(cfg["scale"]), cfg["inv"], False, f32(cfg["dz"]), f32(cfg["sens"]), neutral)

        # ---- FixedUpdate
        throttle = f32(st["mainThrottle"] * 2.0 - 1.0)
        if not locked("THROTTLE"):
            if keydown_state("THROTTLE_UP"):
                throttle = min(1.0, f32(throttle + 1.0 * dt))
            elif keydown_state("THROTTLE_DOWN"):
                throttle = max(-1.0, f32(throttle - 1.0 * dt))
            throttle = min(max(f32(throttle + axis("AXIS_THROTTLE_INC") * dt), -1.0), 1.0)
            axisThrottle = axis("AXIS_THROTTLE")
            if refocus:
                lastAxisThrottle = axisThrottle
                refocus = False
                throttleFocus = False
            if (axisThrottle != lastAxisThrottle) if throttleFocus else (abs(f32(axisThrottle - lastAxisThrottle)) > 0.25):
                throttle = axisThrottle
                lastAxisThrottle = throttle
                throttleFocus = True

        mod = keydown_state("MODIFIER_KEY")

        def rot_axis(lockname, axisname, key_minus, key_plus, trimname, valname, precname, ):
            if not locked(lockname):
                st[valname] = axis(axisname)
                if mod:
                    if keydown_state(key_minus):
                        st[trimname] = min(max(f32(st[trimname] - f32(0.1) * dt), -1.0), 1.0)
                    if keydown_state(key_plus):
                        st[trimname] = min(max(f32(st[trimname] + f32(0.1) * dt), -1.0), 1.0)
                else:
                    if precisionMode:
                        if keydown_state(key_minus):
                            prec[precname] = max(-1.0, f32(prec[precname] - sens_kb * dt))
                        elif keydown_state(key_plus):
                            prec[precname] = min(1.0, f32(prec[precname] + sens_kb * dt))
                        else:
                            prec[precname] = st[valname]
                        st[valname] = prec[precname]
                    else:
                        st[valname] = -1.0 if keydown_state(key_minus) else (1.0 if keydown_state(key_plus) else st[valname])
                        prec[precname] = st[valname]
            else:
                st[valname] = 0.0

        rot_axis("ROLL", "AXIS_ROLL", "ROLL_LEFT", "ROLL_RIGHT", "rollTrim", "roll", "roll")
        rot_axis("PITCH", "AXIS_PITCH", "PITCH_DOWN", "PITCH_UP", "pitchTrim", "pitch", "pitch")
        rot_axis("YAW", "AXIS_YAW", "YAW_LEFT", "YAW_RIGHT", "yawTrim", "yaw", "yaw")
        if mod and "X" in keydown:
            if not locked("PITCH"):
                st["pitchTrim"] = 0.0
            if not locked("ROLL"):
                st["rollTrim"] = 0.0
            if not locked("YAW"):
                st["yawTrim"] = 0.0
            if not locked("WHEEL_STEER"):
                st["wheelSteerTrim"] = 0.0
            if not locked("WHEEL_THROTTLE"):
                st["wheelThrottleTrim"] = 0.0

        def process(axisname, key_plus, key_minus, key, pkey):
            v = axis(axisname)
            if precisionMode:
                if keydown_state(key_minus):
                    prec[pkey] = max(-1.0, f32(prec[pkey] - sens_kb * dt))
                elif keydown_state(key_plus):
                    prec[pkey] = min(1.0, f32(prec[pkey] + sens_kb * dt))
                else:
                    prec[pkey] = v
                v = prec[pkey]
            else:
                v = -1.0 if keydown_state(key_minus) else (1.0 if keydown_state(key_plus) else v)
                prec[pkey] = v
            st[key] = v

        if not locked("LINEAR"):
            st["X"] = f32(0.0 - st["X"])
            st["Y"] = f32(0.0 - st["Y"])
            st["Z"] = f32(0.0 - st["Z"])
            process("AXIS_TRANSLATE_X", "TRANSLATE_RIGHT", "TRANSLATE_LEFT", "X", "X")
            process("AXIS_TRANSLATE_Y", "TRANSLATE_DOWN", "TRANSLATE_UP", "Y", "Y")
            st["Y"] = f32(0.0 - st["Y"])
            process("AXIS_TRANSLATE_Z", "TRANSLATE_FWD", "TRANSLATE_BACK", "Z", "Z")
        else:
            st["X"] = st["Y"] = st["Z"] = 0.0
        st["mainThrottle"] = throttle

        def wheel(lockname, axisname, key_minus, key_plus, trimname, valname, precname):
            # wheel steer: RIGHT is the minus key; wheel throttle: DOWN is the minus key; trims: minus first
            rot_axis(lockname, axisname, key_minus, key_plus, trimname, valname, precname)

        wheel("WHEEL_STEER", "AXIS_WHEEL_STEER", "WHEEL_STEER_RIGHT", "WHEEL_STEER_LEFT", "wheelSteerTrim", "wheelSteer", "ws")
        wheel("WHEEL_THROTTLE", "AXIS_WHEEL_THROTTLE", "WHEEL_THROTTLE_DOWN", "WHEEL_THROTTLE_UP", "wheelThrottleTrim", "wheelThrottle", "wt")
        # custom axes: default AxisKeyBindings never bind anything: 0 (idTag None -> neutral) unless the host sets them (not here)
        if not locked("CUSTOM_ACTION_GROUPS"):
            for i in range(4):
                custom[i] = 0.0
                prec_custom[i] = 0.0
        # ---- OnRawAxisInput(state) hook happens here (no listener in this fixture), then the post-processing
        mt = f32((st["mainThrottle"] + 1.0) * f32(0.5))
        r = f32(st["roll"] + st["rollTrim"])
        p = f32(st["pitch"] + st["pitchTrim"])
        y = f32(st["yaw"] + st["yawTrim"])
        ws = f32(st["wheelSteer"] + st["wheelSteerTrim"])
        wt = f32(st["wheelThrottle"] + st["wheelThrottleTrim"])
        st["mainThrottle"], st["roll"], st["pitch"], st["yaw"], st["wheelSteer"], st["wheelThrottle"] = mt, r, p, y, ws, wt
        st["X"] = f32(0.0 - st["X"])
        st["Z"] = f32(0.0 - st["Z"])
        out_frames.append(dict(st))
    return {"inputs": inputs, "states": out_frames, "bindings": {k: list(v) for k, v in kb.items()}, "axis_cfg": axis_cfg}


# ---------------------------------------------------------------------------------------------- Mouse
def mouse_run(seed, frames, speed, screen_h=1080):
    """Mouse.Update + MouseButton.Update + TapRoutine (a Python generator = the Unity coroutine), driven like Unity: every Update first,
    then the coroutine pass; a routine started during Update ran its first segment already and is first resumed next frame."""
    rng = random.Random(seed)
    s = dict(button=False, down=False, up=False, tap=False, doubleTap=False, abort=False, doubleClickTime=0.0, isTapStarted=False)
    now = [0.0]
    cur = {"up": False, "down": False}
    held = False
    screen = [0.0, 0.0]
    last = [0.0, 0.0]
    p_at_down = [0.0, 0.0]
    drag = [0.0, 0.0]
    delta = [0.0, 0.0]

    def tap_routine():
        s["isTapStarted"] = True
        end = f32(now[0] + speed)
        tapped = False
        while not s["abort"] and not (now[0] >= end):
            if cur["up"]:
                s["tap"] = True
                tapped = True
                yield
                s["tap"] = False
                break
            yield
        if tapped:
            end = f32(now[0] + speed)
            while not s["abort"] and not (now[0] >= end):
                if cur["down"]:
                    s["doubleTap"] = True
                    s["tap"] = False
                    s["doubleClickTime"] = f32(now[0] + speed)
                    break
                yield
        s["abort"] = False
        s["isTapStarted"] = False

    co = None
    t = 0.0
    out = []
    for fno in range(frames):
        t = f32(t + f32(rng.choice([0.016, 0.033, 0.05, 0.1])))
        now[0] = t
        down = up = False
        if not held and rng.random() < 0.35:
            down = True
            held = True
        elif held and rng.random() < 0.5:
            up = True
            held = False
        clear = rng.random() < 0.03
        mx = f32(rng.uniform(0, 1920))
        my = f32(rng.uniform(0, screen_h))
        if rng.random() < 0.2:
            mx, my = last[0], f32(screen_h - last[1])
        cur.update(up=up, down=down)
        started_now = False
        # ---- Mouse.Update: Left.Update() first (reads the previous frame's screenPos), then the cursor
        if clear:  # ClearMouseState is called by game code between frames
            s.update(doubleClickTime=t, tap=False, doubleTap=False, down=False, up=False, button=False, abort=True)
        if s["abort"]:
            if not s["isTapStarted"]:
                s["abort"] = False
        else:
            s["down"], s["up"], s["button"] = down, up, held
            if down:
                p_at_down = list(screen)
            elif held:
                drag = [f32(screen[0] - p_at_down[0]), f32(screen[1] - p_at_down[1])]
            if not s["isTapStarted"]:
                s["tap"] = False
                s["doubleTap"] = False
                if down:
                    co = tap_routine()
                    started_now = True
                    try:
                        next(co)
                    except StopIteration:
                        co = None
        screen = [mx, f32(screen_h - my)]
        delta = [f32(screen[0] - last[0]), f32(screen[1] - last[1])]
        last = list(screen)
        # ---- coroutine pass
        if co is not None and not started_now:
            try:
                next(co)
            except StopIteration:
                co = None
        sqr = f32(drag[0] * drag[0] + drag[1] * drag[1])
        out.append({"t": t, "down": down, "up": up, "held": held, "clear": clear, "mx": mx, "my": my,
                    "tap": s["tap"], "doubleTap": s["doubleTap"], "isTapStarted": s["isTapStarted"], "abort": s["abort"],
                    "bDown": s["down"], "bUp": s["up"], "bButton": s["button"], "dclick": s["doubleClickTime"],
                    "dragX": drag[0], "dragY": drag[1], "wasDragging": (s["button"] or s["up"]) and sqr > 400.0,
                    "screenX": screen[0], "screenY": screen[1], "deltaX": delta[0], "deltaY": delta[1],
                    "isMoving": f32(delta[0] * delta[0] + delta[1] * delta[1]) > f32(0.01)})
    return out


# ---------------------------------------------------------------------------------------------- misc: field order, valid keys, SpaceNavigator
def public_fields():
    out = []
    for line in GS:
        m = re.match(r"^	public static ([\w<>\.]+) (\w+)(?: = (.+))?;$", line)
        if m:
            out.append(m.group(2))
    return out


def valid_keys():
    vals = sorted(v for _n, v in KEYCODES)
    unbind = {KC["Mouse0"], KC["Mouse1"], KC["Mouse2"]}
    vals = [v for v in vals if v not in unbind]
    return vals + [i + 128 for i in range(127)]


def safe(fn):
    try:
        return f32(fn())
    except ValueError:
        return None


def quat_cases(seed, count):
    rng = random.Random(seed)
    out = []
    for _ in range(count):
        v = [rng.uniform(-1, 1) for _ in range(4)]
        n = math.sqrt(sum(x * x for x in v)) or 1.0
        q = [f32(x / n) for x in v]  # x y z w
        x, y, z, w = q
        pitch = safe(lambda: math.atan2(f32(2.0 * (y * z + w * x)), f32(w * w - x * x - y * y + z * z)))
        yaw = safe(lambda: math.asin(f32(-2.0 * (x * z - w * y))))
        roll = safe(lambda: math.atan2(f32(2.0 * (x * y + w * z)), f32(w * w + x * x - y * y - z * z)))
        out.append({"q": q, "pitch": pitch, "yaw": yaw, "roll": roll})
    return out


def spacenav_cases(seed, count):
    rng = random.Random(seed)
    out = []
    for _ in range(count):
        t = [rng.uniform(-350, 350) for _ in range(3)]
        num = f32(rng.choice([1.0, 0.05, 50.0, f32(rng.uniform(0.01, 100.0))]))
        lockx, locky, lockz, lockall = [rng.random() < 0.15 for _ in range(4)]
        x = 0.0 if (lockx or lockall) else f32(t[0])
        y = 0.0 if (locky or lockall) else f32(t[1])
        z = 0.0 if (lockz or lockall) else f32(0.0 - f32(t[2]))
        tx = f32(f32(x * num) * f32(0.0001))
        ty = f32(f32(y * num) * f32(0.0001))
        tz = f32(f32(z * num) * f32(0.0001))
        rot_in = [rng.uniform(-1, 1) for _ in range(3)]
        angle = rng.uniform(-300, 300)
        rnum = f32(rng.choice([1.0, 0.0, 5.0, f32(rng.uniform(0.0, 5.0))]))
        ang = f32(f32(angle) * rnum * f32(0.0008))
        rl = [rng.random() < 0.15 for _ in range(4)]
        ax = 0.0 if (rl[0] or rl[3]) else f32(0.0 - f32(rot_in[0]))
        ay = 0.0 if (rl[1] or rl[3]) else f32(0.0 - f32(rot_in[1]))
        az = 0.0 if (rl[2] or rl[3]) else f32(rot_in[2])
        out.append({"t": t, "num": num, "locks": [lockx, locky, lockz, lockall], "expect": [tx, ty, tz],
                    "rot": rot_in, "angle": angle, "rnum": rnum, "rlocks": rl, "expect_angle": ang, "expect_axis": [ax, ay, az]})
    return out


def main():
    ref = {}
    ref["control_types"] = {
        "members": [[n, str(v)] for n, v in CONTROL_TYPES],
        "tostring": [],
        "zero_names": [n for n, v in CONTROL_TYPES if v == 0],
    }
    rng = random.Random(5001)
    single = [v for n, v in CONTROL_TYPES if v and (v & (v - 1)) == 0]
    vals = set()
    for n, v in CONTROL_TYPES:
        vals.add(v)
    for _ in range(300):
        v = 0
        for _ in range(rng.randint(0, 5)):
            v |= rng.choice(single)
        vals.add(v)
    vals.add(M64)
    vals.add(1 << 63)
    vals.add(0xFFFFFFFFFFFFFFF | (1 << 62))
    for v in sorted(vals):
        ref["control_types"]["tostring"].append([str(v), flags_tostring_controltypes(v)])
    ref["keycode"] = {"members": [[n, v] for n, v in KEYCODES], "kce": []}
    toks = ["W", "w", "None", "160", "27", "-1", "  Q ", "W, A", "Joystick1Button3", "ö", "#", "1", "Alpha1", "999999999999", "", "KeypadPlus",
            "+5", "LeftShift", "Mouse2", "a1", "Space", "space", "ö", "123abc", "0", "300", "255", "-128", "Escape", "LeftAlt, RightAlt",
            "F13", "Backslash", "Backspace", "Return", "Delete", "Equals"]
    for t in toks:
        r = kce(t)
        ref["keycode"]["kce"].append([t, None if r is None else {"code": r[0], "name": r[1], "tostring": r[2]}])
    ops = lock_sequence(5002, 400)
    ref["locks"] = {"ops": ops, "results": run_locks(ops)}
    ref["control_locks"] = control_locks_table()
    ref["layouts"] = layouts()
    ref["defaults"] = default_values()
    ref["default_text"] = default_text()
    ref["axis"] = axis_cases(5003, 600)
    ref["flight"] = flight_reference(5004, 360)
    ref["public_fields"] = public_fields()
    ref["valid_keys"] = valid_keys()
    ref["quat"] = quat_cases(5006, 400)
    ref["spacenav"] = spacenav_cases(5007, 200)
    ref["mouse"] = {"speed": f32(0.2), "frames": mouse_run(5005, 400, f32(0.2))}
    with open(OUT, "w", newline="\n") as f:
        json.dump(ref, f, separators=(",", ":"))
    print(OUT, os.path.getsize(OUT))


if __name__ == "__main__":
    main()
