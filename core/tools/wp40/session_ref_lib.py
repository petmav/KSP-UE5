#!/usr/bin/env python3
"""WP-40b: independent reference simulator of the EditorLogic placement loop (library half; scenarios and the JSON writer are in session_reference.py).

Written from the decompiled C# (EditorLogic.cs 1236-2366 SetupFSM, the helpers it calls, EditorLogicBase.cs, EditorToolsUI.cs, KerbalFSM.cs, Mouse.cs,
GameSettings.cs defaults, PartLoader.cs / Part.cs for the part model, GizmoOffset.cs / GizmoRotate.cs). It shares NO code with kspcore; it reuses the
numeric primitives of tools/wp40/editor_reference.py (the WP-40 reference: Mono float rule, per-operation float32 for native Unity maths) and
the WP-12 native FromToRotation of tools/wp12/pqs_ref_math.py.

Inputs (original game data): harness/captures/out2/part_database.json (attach nodes, attach rules, stackSymmetry) and the stock crafts of
harness/inputs/Ships. Scene queries (camera rays, Physics.Raycast, attach node icon bounds, FindPartSurface, UI hover) are SEAMS: the simulator answers
them with analytic test fixtures (spheres around the parts, a pinhole-free ray table) and RECORDS every call and its answer in call order; the C++ test
replays the recorded answers and checks that the port asks the same questions in the same order.

Native Unity functions whose result is not captured against the game (LookRotation, Euler, eulerAngles, Bounds.Contains, Transform.localPosition /
localRotation derivation) are UNVERIFIED, exactly like in WP-40; the sessions that depend on them are listed in the output (`unverified`).
"""
import hashlib
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "wp12"))
import editor_reference as E  # noqa: E402
import pqs_ref_math as PM  # noqa: E402

F = E.F
f = E.f
bits = E.bits
ZERO, UP, FORWARD, RIGHT, ONE, QID = E.ZERO, E.UP, E.FORWARD, E.RIGHT, E.ONE, E.QID
vadd, vsub, vneg, vmul, vdiv, vscale, dot, sqr_mag, mag = E.vadd, E.vsub, E.vneg, E.vmul, E.vdiv, E.vscale, E.dot, E.sqr_mag, E.mag
vnormalized, project_on_plane = E.vnormalized, E.project_on_plane
qmul, qmul_vec, conj, normalize_safe = E.qmul, E.qmul_vec, E.conj, E.normalize_safe
angle_axis = E.angle_axis
USED = set()     # the UNVERIFIED native Unity functions a session reached


def look_rotation(forward, up):
    USED.add("Quaternion.LookRotation")
    return E.look_rotation(forward, up)


def euler(deg):
    USED.add("Quaternion.Euler")
    return E.euler(deg)
v3 = E.v3

STACK, SRF_ATTACH = 0, 1           # AttachModes
RADIAL, MIRROR = 0, 1              # SymmetryMethod
SELF, WORLD = 1, 0                 # UnityEngine.Space
DEFAULT_EQUIVALENT = 8519681       # LayerUtil.DefaultEquivalent
LAYER_MASK = 67109122 + DEFAULT_EQUIVALENT   # EditorLogic.cs:498


def fl(x):
    return float(x)


def clamp01(x):
    x = float(x)
    return F(0.0) if x < 0.0 else (F(1.0) if x > 1.0 else F(x))


def vlerp(a, b, t):
    t = float(clamp01(t))
    return tuple(f(float(a[i]) + (float(b[i]) - float(a[i])) * t) for i in range(3))


def vcross(a, b):
    return (f(float(a[1]) * float(b[2]) - float(a[2]) * float(b[1])), f(float(a[2]) * float(b[0]) - float(a[0]) * float(b[2])),
            f(float(a[0]) * float(b[1]) - float(a[1]) * float(b[0])))


def vdist(a, b):
    d = vsub(a, b)
    return f(math.sqrt(float(sqr_mag(d))))


def veq_approx(a, b):
    """Vector3.operator== (sqrMagnitude of the difference < 9.99999944E-11)"""
    return float(sqr_mag(vsub(a, b))) < float(F(9.99999944e-11))


def vexact(a, b):
    return all(float(x) == float(y) for x, y in zip(a, b))


def hexf(x):
    return "%08x" % bits(x)


def from_to_rotation(a, b):
    q = PM.native_from_to_rotation(tuple(float(x) for x in a), tuple(float(x) for x in b))
    return tuple(F(c) for c in q)


def mathf_floor(x):
    return F(math.floor(float(x)))


def bearing_degrees(v1, v2, up_axis):
    if veq_approx(v1, v2):
        return F(0.0)
    d = float(dot(v1, v2))
    acos = f(math.acos(max(-1.0, min(1.0, d))) if abs(d) <= 1.0 else float("nan"))
    sign = F(1.0) if float(dot(vcross(v1, v2), up_axis)) >= 0.0 else F(-1.0)
    return f(float(acos) * float(sign) * float(F(57.29578)))


class Node:
    def __init__(self, nid, ntype, pos, ori, size):
        self.id = nid
        self.type = ntype
        self.position = pos
        self.orientation = ori
        self.size = size
        self.offset = ZERO
        self.attached = None
        self.icon = False
        self.srf_mesh = ""
        self.owner = None
        self.orig_orientation = ori

    def copy(self):
        n = Node(self.id, self.type, self.position, self.orientation, self.size)
        n.offset = self.offset
        n.srf_mesh = self.srf_mesh
        n.orig_orientation = self.orig_orientation
        return n


class Attachment:
    def __init__(self):
        self.possible = False
        self.collision = False
        self.mode = STACK
        self.potentialParent = None
        self.caller = None
        self.callerNode = None
        self.otherNode = None
        self.position = ZERO
        self.rotation = (F(0), F(0), F(0), F(0))


class Part:
    def __init__(self, sim, name):
        self.sim = sim
        self.id = sim.new_id()
        self.name = name
        db = sim.db
        info = db["parts"][name]
        rows = db["nodes"].get(name, [])
        self.pos = ZERO
        self.rot = QID
        self.parent = None
        self.children = []
        self.sym = []
        self.potentialParent = None
        self.attPos0 = ZERO
        self.attRot0 = QID
        self.attPos = ZERO
        self.attRot = QID
        self.initRot = QID
        self.attachMode = STACK
        self.symMethod = RADIAL
        self.nodes = []
        self.srf = None
        for r in rows[:-1]:
            self.nodes.append(self._node(r))
        if rows:
            self.srf = self._node(rows[-1])
        cfg_rules = db["cfg"].get(name, {}).get("attachRules", ["1,1,1,1,0"])[0].split(",")
        self.allowRoot = (cfg_rules[7].strip() == "1") if len(cfg_rules) >= 8 else True
        for n in self.nodes + ([self.srf] if self.srf is not None else []):
            n.owner = self
        if self.srf is not None and self.srf.id == "":
            self.srf.owner = None     # a part without node_attach has the default-constructed srfAttachNode (no owner): PartLoader.cs / the C9 capture
        rules = [x.strip() for x in info["attachRules"].split(",")]
        keys = ["stack", "srfAttach", "allowStack", "allowSrfAttach", "allowCollision", "allowDock", "allowRotate"]
        self.rules = {k: rules[i] == "1" for i, k in enumerate(keys)}
        self.stackSymmetry = int(info["stackSymmetry"])
        self.isAttached = False
        self.destroyed = False
        self.crew = int(info["CrewCapacity"])

    @staticmethod
    def _node(r):
        g = E.from_hex
        return Node(r["id"], r["nodeType"], (g(r["position_x_hex"]), g(r["position_y_hex"]), g(r["position_z_hex"])),
                    (g(r["orientation_x_hex"]), g(r["orientation_y_hex"]), g(r["orientation_z_hex"])), int(r["size"]))

    # ---- Transform ----
    def T(self):
        return {"pos": self.pos, "rot": self.rot, "scale": ONE}

    def tp(self, p):
        return E.t_point(self.T(), p)

    def td(self, d):
        return E.t_dir(self.T(), d)

    def itp(self, p):
        return E.t_inv_point(self.T(), p)

    def up(self):
        return E.t_up(self.T())

    def forward(self):
        return E.t_forward(self.T())

    def set_rotation(self, q):
        self.rot = normalize_safe(q)

    def local_position(self):
        USED.add("Transform.localPosition / localRotation (derived from the parent matrix)")
        return E.t_inv_point(self.parent.T(), self.pos) if self.parent is not None else self.pos

    def local_rotation(self):
        return qmul(conj(self.parent.rot), self.rot) if self.parent is not None else self.rot

    def set_local_position(self, v):
        self.pos = E.t_point(self.parent.T(), v) if self.parent is not None else v

    def set_local_rotation(self, q):
        self.rot = normalize_safe(qmul(self.parent.rot, q) if self.parent is not None else q)

    # ---- tree ----
    def local_root(self):
        p = self
        while p.parent is not None:
            p = p.parent
        return p

    def find_node_by_part(self, other):
        if other is None:
            return None
        for n in self.nodes:
            if n.attached is other:
                return n
        if self.srf.attached is other:
            return self.srf
        return None

    def find_node(self, nid):
        for n in self.nodes:
            if n.id == nid:
                return n
        return None

    def get_sym(self, index):
        index %= len(self.sym) + 1
        return self if index == 0 else self.sym[index - 1]

    def all_nodes(self):
        return self.nodes   # Part.attachNodes (the surface node is the separate srfAttachNode)


def find_parts_in_children(part, out=None):
    if out is None:
        out = []
    out.append(part)
    for c in part.children:
        find_parts_in_children(c, out)
    return out


def in_hierarchy(part, tgt):
    if tgt in part.children:
        return True
    return any(in_hierarchy(c, tgt) for c in part.children)


def clear_attach_nodes(part, other):
    if other is None or other.destroyed:
        return
    for n in other.nodes:
        if not (n.attached is not part):
            n.attached = None
            break
    if other.srf.attached is part:
        other.srf.attached = None
    for n in part.nodes:
        if not (n.attached is not other):
            n.attached = None
            break
    if part.srf.attached is other:
        part.srf.attached = None


# ----------------------------------------------------------------------------------------------------------------------------------------
# KerbalFSM (KerbalFSM.cs, KFSMState.cs, KFSMEvent.cs)
# ----------------------------------------------------------------------------------------------------------------------------------------
MANUAL, UPDATE, FIXED, LATE = 0, 1, 2, 4


class KState:
    def __init__(self, name):
        self.name = name
        self.events = []
        self.t_enter = F(0)
        self.frame_enter = 0
        self.on_enter = lambda frm: None
        self.on_update = lambda: None
        self.on_leave = lambda to: None


class KEvent:
    def __init__(self, name):
        self.name = name
        self.goto = None
        self.mode = UPDATE
        self.on_event = lambda: None
        self.check = lambda st: False


class KFsm:
    def __init__(self, clock):
        self.clock = clock
        self.states = []
        self.current = None
        self.last_event = None
        self.started = False
        self.cur_name = ""
        self.on_event_called = lambda ev: None
        self.on_state_change = lambda a, b, ev: None

    def add_state(self, st):
        self.states.append(st)

    def add_event(self, ev, to_states):
        for st in reversed(to_states):
            for s in reversed(self.states):
                if s is st:
                    st.events.append(ev)
                    break
            else:
                raise Exception("FSM Error: Cannot add events to state")

    def add_event_excluding(self, ev, exc):
        for s in reversed(self.states):
            if not any(e is s for e in exc):
                s.events.append(ev)

    def frames_in_current(self):
        return self.clock.frame - self.current.frame_enter

    def run_event(self, ev):
        if not self.started:
            raise Exception("FSM Error: Cannot switch states before starting.")
        if ev not in self.current.events:
            return
        self.on_event_called(ev)
        ev.on_event()
        go = ev.goto
        self.last_event = ev
        self.cur_name = go.name
        self.current.on_leave(go)
        last = self.current
        self.current = go
        self.current.t_enter = self.clock.time
        self.current.frame_enter = self.clock.frame
        self.on_state_change(last, go, ev)
        self.current.on_enter(last)

    def start(self, st):
        self.current = st
        st.on_enter(None)
        st.t_enter = self.clock.time
        st.frame_enter = self.clock.frame
        self.cur_name = st.name
        self.started = True

    def update(self):
        if self.started:
            self.current.on_update()
            self._update(UPDATE)

    def late_update(self):
        if self.started:
            self._update(LATE)

    def _update(self, mode):
        i = 0
        while i < len(self.current.events):
            ev = self.current.events[i]
            if ev.mode == mode and ev.check(self.current):
                self.run_event(ev)
            i += 1


class Clock:
    def __init__(self):
        self.frame = 0
        self.time = F(0.0)
        self.dt = F(0.02)


# ----------------------------------------------------------------------------------------------------------------------------------------
# input (UnityEngine.Input as scripted by the session, Mouse.cs)
# ----------------------------------------------------------------------------------------------------------------------------------------
class MouseButton:
    def __init__(self, idx):
        self.idx = idx
        self.down = self.up = self.button = False
        self.p_at_down = (F(0), F(0))
        self.drag = (F(0), F(0))

    def update(self, inp, screen_pos):
        self.down = inp.mb_down[self.idx]
        self.up = inp.mb_up[self.idx]
        self.button = inp.mb_held[self.idx]
        if self.down:
            self.p_at_down = screen_pos
        elif self.button:
            self.drag = (f(float(screen_pos[0]) - float(self.p_at_down[0])), f(float(screen_pos[1]) - float(self.p_at_down[1])))

    def was_dragging(self, delta=400.0):
        if not self.button and not self.up:
            return False
        d = f(float(self.drag[0]) * float(self.drag[0]) + float(self.drag[1]) * float(self.drag[1]))
        return float(d) > float(F(delta))


class Input:
    def __init__(self):
        self.mouse = (F(0), F(0), F(0))
        self.mb_down = [False] * 3
        self.mb_up = [False] * 3
        self.mb_held = [False] * 3
        self.kd = set()
        self.ku = set()
        self.kh = set()
        self.screen_pos = (F(0), F(0))
        self.left = MouseButton(0)
        self.right = MouseButton(1)
        self.middle = MouseButton(2)
        self.screen_height = 1080

    def key(self, k):
        return k in self.kh

    def key_down(self, k):
        return k in self.kd

    def key_up(self, k):
        return k in self.ku

    def mouse_update(self):
        # Mouse.cs Update: the buttons first (with the screenPos of the previous frame), then screenPos
        self.left.update(self, self.screen_pos)
        self.right.update(self, self.screen_pos)
        self.middle.update(self, self.screen_pos)
        self.screen_pos = (self.mouse[0], f(float(self.screen_height) - float(self.mouse[1])))


# GameSettings.cs:1316-1354 defaults (primary key of each editor binding)
KB = {"pitchUp": "S", "pitchDown": "W", "yawLeft": "A", "yawRight": "D", "rollLeft": "Q", "rollRight": "E", "resetRotation": "Space",
      "modePlace": "Alpha1", "modeOffset": "Alpha2", "modeRotate": "Alpha3", "modeRoot": "Alpha4", "coordSystem": "F",
      "toggleSymMethod": "R", "toggleSymMode": "X", "toggleAngleSnap": "C", "fineTweak": "LeftShift"}


# ----------------------------------------------------------------------------------------------------------------------------------------
# editor simulator
# ----------------------------------------------------------------------------------------------------------------------------------------
class Gizmo:
    pass


class Sim:
    CAMERA = v3(-12.0, 9.0, -14.0)

    def __init__(self, db, facility="VAB"):
        Sim.offset_space = WORLD     # GizmoOffset.coordSpace / GizmoRotate.coordSpace are statics: a fresh process starts with their defaults
        Sim.rotate_space = SELF
        self.db = db
        self.facility = facility
        self.clock = Clock()
        self.inp = Input()
        self.counter = 0
        self.parts = []                  # every part ever created (creation order)
        self.ship = []                   # ship.parts
        self.rootPart = None
        self.selectedPart = None
        self.backups = []
        self.undoLevel = 0
        self.attachment = None
        self.cPartAttachments = []
        self.symmetryMethod = RADIAL
        self.symmetryMethodTmp = RADIAL
        self.symmetryMode = 0
        self.symmetryModeTmp = 0
        self.symmetryModeBeforeNodeAttachment = -1
        self.tmpSymMethodInUse = False
        self.symmetryCoordSpace = SELF
        self.vesselRotation = QID
        self.srfAttachAngleSnap = F(15.0)
        self.srfAttachAngleSnapFine = F(5.0)
        self.allowSrf = True
        self.allowNode = True
        self.selPartGrabOffset = ZERO
        self.dragPlaneCenter = ZERO
        self.srfAttachCursorOffset = ZERO
        self.dragSharpness = F(0.3)
        self.skipPartAttach = False
        self.coroutines = []
        self.isCurrentPartFlag = False
        self.stackRotation = QID
        self.partTweaked = False
        self.symUpdateMode = 0
        self.symUpdateParent = None
        self.symUpdateNode = None
        self.gizmoPivot = ZERO
        self.gizmoAttRot = QID
        self.gizmoAttRot0 = QID
        self.gizmoOffset = None
        self.gizmoRotate = None
        self.rootCandidates = []
        self.constructionMode = 0
        self.toolsMode = 0
        self.editorScreen = 0
        self.frozen = set()
        self.use_angle_snap = False
        self.undoLimit = 32
        self.undoIndexAtLastSave = 0
        self.locks = {}
        self.trace = []
        self.seam = []
        self.rays = {}
        self.cheat_clip = False
        self.fine_offset_threshold = F(20.0)
        self.listeners_mode = []
        self.restarting = False
        self.fsm = KFsm(self.clock)
        self.fsm.on_event_called = lambda ev: self.trace.append("ev:" + ev.name)
        self.fsm.on_state_change = lambda a, b, ev: self.trace.append("st:%s>%s" % (a.name, b.name))
        self.setup_fsm()

    # ---- bookkeeping ----
    def new_id(self):
        self.counter += 1
        return self.counter

    def log(self, s):
        self.trace.append(s)

    def new_part(self, name):
        p = Part(self, name)
        self.parts.append(p)
        return p

    def alive(self, p):
        return p is not None and not p.destroyed

    def ge(self, name, *args):
        self.trace.append("ge:" + name + ("(" + ",".join(str(a) for a in args) + ")" if args else ""))

    def pid(self, p):
        return "-" if p is None else str(p.id)

    # ---- locks (InputLockManager) ----
    def set_lock(self, lock_id, mask):
        self.locks[lock_id] = mask

    def remove_lock(self, lock_id):
        self.locks.pop(lock_id, None)

    def lock_mask(self):
        m = 0
        for v in self.locks.values():
            m |= v
        return m

    def is_unlocked(self, ctrl):
        return (self.lock_mask() & ctrl) == 0

    def is_locked(self, ctrl):
        return (self.lock_mask() & ctrl) != 0

    # ---- seams (recorded; analytic fixtures) ----
    def seam_ray(self, sp):
        key = "%s,%s,%s" % (hexf(sp[0]), hexf(sp[1]), hexf(sp[2]))
        cam = self.CAMERA
        if key in self.rays:
            o, d = self.rays[key]
        else:
            tgt = (f(float(sp[0]) / 100.0), f(float(sp[1]) / 100.0), sp[2])
            o, d = cam, vnormalized(vsub(tgt, cam))
        self.seam.append(["ray", [hexf(c) for c in sp], [hexf(c) for c in o], [hexf(c) for c in d]])
        return o, d

    def world_to_screen(self, p):
        return (f(float(p[0]) * 100.0), f(float(p[1]) * 100.0), p[2])

    RADIUS = {"mk1pod.v2": 0.9, "radialDecoupler": 0.45, "liquidEngine": 0.9}

    def _radius(self, p):
        return F(self.RADIUS.get(p.name, 0.8))

    def _ray_sphere(self, o, d, center, r):
        oc = vsub(o, center)
        b = float(dot(oc, d))
        c = float(dot(oc, oc)) - float(r) * float(r)
        disc = b * b - c
        if disc < 0.0:
            return None
        s = math.sqrt(disc)
        t = -b - s
        if t < 0.0:
            t = -b + s
        if t < 0.0:
            return None
        return F(t)

    def seam_raycast(self, o, d, maxd, mask):
        best = None
        for p in self.parts:
            if p.destroyed:
                continue
            layer = 0 if p in self.ship else 1
            if not (mask >> layer) & 1:
                continue
            t = self._ray_sphere(o, d, p.pos, self._radius(p))
            if t is not None and float(t) <= float(maxd) and (best is None or float(t) < float(best[0])):
                best = (t, p)
        if best is None:
            self.seam.append(["raycast", [hexf(c) for c in o], [hexf(c) for c in d], int(mask), None])
            return None
        t, p = best
        point = vadd(o, vmul(d, t))
        normal = vnormalized(vsub(point, p.pos))
        self.seam.append(["raycast", [hexf(c) for c in o], [hexf(c) for c in d], int(mask),
                          [p.id, [hexf(c) for c in point], [hexf(c) for c in normal]]])
        return p, point, normal

    def seam_icon(self, part, node, wp):
        centre = part.tp(node.position)
        res = float(vdist(wp, centre)) <= 0.35
        self.seam.append(["icon", part.id, node.id, [hexf(c) for c in wp], bool(res)])
        return res

    def seam_surface(self, p, origin, direction, from_parent, hit_normal):
        o2 = vadd(origin, vmul(from_parent, F(100.0)))
        d2 = vneg(from_parent)
        t = self._ray_sphere(o2, d2, p.pos, self._radius(p))
        num = F(0.0)
        srf = hit_normal
        if t is not None and float(t) <= 110.0:
            point = vadd(o2, vmul(d2, t))
            vec = project_on_plane(vsub(point, origin), direction)
            if float(sqr_mag(vec)) > float(num):
                num = sqr_mag(vec)
                srf = vnormalized(vsub(point, p.pos))
        res = f(math.sqrt(float(num))) if float(num) != 0.0 else F(1.0)
        self.seam.append(["surface", p.id, [hexf(c) for c in origin], [hexf(c) for c in direction], [hexf(c) for c in from_parent],
                          [hexf(c) for c in hit_normal], hexf(res), [hexf(c) for c in srf]])
        return res, srf

    def clip(self, name):
        self.trace.append("clip:" + name)

    def highlight(self, attach, part, reset=False):
        self.trace.append("hl:%s,%d,%d" % (self.pid(part), int(attach), int(reset)))

    # ---- scene parts ----
    def has_flag_decal(self, p):
        return False

    # ----------------------------------------------------------------------------------------------------------------------------------------
    # EditorLogicBase / EditorLogic data helpers
    # ----------------------------------------------------------------------------------------------------------------------------------------
    def display_icons(self, parts, selected, stack, srf, dock):
        for p in parts:
            self._icons(p, stack, srf, dock)
        self._icons(selected, stack, srf, dock)
        self._child_icons(selected)

    def _child_icons(self, part):
        for c in part.children:
            self._icons(c, False, False, False)
            self._child_icons(c)

    def _icons(self, part, stack, srf, dock):
        for n in part.nodes:
            if (stack or n.type != "Stack") and (dock or n.type != "Dock") and n.type != "Surface" and \
                    (not (n.attached is not None) or self.cheat_clip or not (part is not None)):
                n.icon = True
            elif n.icon:
                n.icon = False

    def set_sym_state(self):
        self.restore_symmetry_state()

    def restore_symmetry_state(self):
        if self.tmpSymMethodInUse:
            self.tmpSymMethodInUse = False
            self.symmetryMethod = self.symmetryMethodTmp
            if self.symmetryMethod == MIRROR:
                self.symmetryMode = min(1, self.symmetryModeTmp)
            else:
                self.symmetryMode = self.symmetryModeTmp

    def restore_symmetry_mode_before_node(self):
        self.symmetryMode = self.symmetryModeBeforeNodeAttachment
        self.set_sym_state()
        self.symmetryModeBeforeNodeAttachment = -1

    def first_non_symmetrical_parent(self, p):
        if len(p.sym) != 0:
            return self.first_non_symmetrical_parent(p.parent)
        return p

    # ---- Part copy (Object.Instantiate of the part with its child hierarchy) ----
    def instantiate(self, p):
        orig = find_parts_in_children(p)
        mp = {}
        clones = []
        for o in orig:
            c = Part(self, o.name)
            self.parts.append(c)
            c.pos, c.rot = o.pos, o.rot
            c.attPos0, c.attRot0, c.attPos, c.attRot, c.initRot = o.attPos0, o.attRot0, o.attPos, o.attRot, o.initRot
            c.attachMode, c.symMethod = o.attachMode, o.symMethod
            c.isAttached = o.isAttached
            c.nodes = [n.copy() for n in o.nodes]
            c.srf = o.srf.copy() if o.srf is not None else None
            c.allowRoot = o.allowRoot
            for n in c.nodes + ([c.srf] if c.srf is not None else []):
                n.owner = c
            if c.srf is not None and o.srf.owner is None:
                c.srf.owner = None
            mp[id(o)] = c
            if o in self.frozen:
                self.frozen.add(c)
            clones.append(c)
        for o, c in zip(orig, clones):
            if o is not p and o.parent is not None and id(o.parent) in mp:
                self.set_parent(c, mp[id(o.parent)])
            c.sym = [mp.get(id(s), s) for s in o.sym]
            for k, n in enumerate(o.nodes):
                t = n.attached
                c.nodes[k].attached = mp.get(id(t), t) if t is not None else None
            if o.srf is not None:
                t = o.srf.attached
                c.srf.attached = mp.get(id(t), t) if t is not None else None
        return clones[0]

    def set_parent(self, part, p):
        if p is not None:
            p.children.append(part)
        elif part.parent is not None:
            if part in part.parent.children:
                part.parent.children.remove(part)
        part.parent = p
        part.potentialParent = p

    def clear_parent(self, part):
        part.parent = None
        part.potentialParent = None

    def duplicate_part(self, p):
        return self.instantiate(p)

    # ---- symmetry (EditorLogic.cs:3140-3520) ----
    def delete_symmetry_parts(self):
        self.restore_symmetry_state()
        if not self.alive(self.selectedPart):
            return
        sel = self.selectedPart
        for part in list(sel.sym):
            if self.alive(part) and sel is not part:
                self.fire_part_event("PartSymmetryDeleted", part)
                self.detach_part(part)
                self.delete_part(part, True)
        self.wipe_symmetry(sel)

    def wipe_symmetry(self, sel):
        sel.sym = []
        lst = find_parts_in_children(sel)
        hs = set(id(x) for x in lst)
        for part in lst:
            j = 0
            num = len(part.sym)
            while j < num:
                if id(part.sym[j]) not in hs:
                    del part.sym[j]
                    j -= 1
                    num -= 1
                j += 1

    def clean_symmetry(self):
        for part in self.ship:
            j = 0
            num = len(part.sym)
            while j < num:
                if part.sym[j] is None or part.sym[j].destroyed:
                    del part.sym[j]
                    j -= 1
                    num -= 1
                j += 1

    def create_symmetry(self, mode, attach):
        part = self.selectedPart
        if not self.tmpSymMethodInUse and (len(part.sym) != mode or part.symMethod != self.symmetryMethod):
            self.delete_symmetry_parts()
        part.symMethod = self.symmetryMethod
        if mode == 0:
            if attach.mode == SRF_ATTACH and self.symmetryModeBeforeNodeAttachment >= 0 and len(part.sym) == 0:
                self.restore_symmetry_mode_before_node()
            return
        pp = part.potentialParent
        if len(pp.sym) == 0:
            if vexact((part.pos[0], F(0.0), part.pos[2]), (pp.pos[0], F(0.0), pp.pos[2])):
                self.delete_symmetry_parts()
                return
            if attach.mode == STACK and (pp.stackSymmetry != mode or (self.facility == "VAB" and self.symmetryMethod == MIRROR)):
                self.symmetryModeBeforeNodeAttachment = self.symmetryMode
                self.symmetryMode = pp.stackSymmetry
                self.set_sym_state()
                return
        if len(part.sym) == 0:
            for i in range(mode):
                self.will_be_copied(part, True)
                part2 = self.duplicate_part(self.selectedPart)
                part.sym.append(part2)
                part2.sym = [self.selectedPart]
                part2.symMethod = self.symmetryMethod
                self.on_copy(part2, part, True)
                if len(pp.sym) == 0:
                    part2.potentialParent = pp
                    continue
                if len(pp.sym) == mode and pp.symMethod == self.symmetryMethod:
                    part2.potentialParent = pp.get_sym(i + 1)
                    continue
                self.symmetryMode = len(pp.sym)
                if part.symMethod != pp.symMethod:
                    self.restore_symmetry_state()
                    self.symmetryMethod = pp.symMethod
                    self.fire_sym_method_change(self.symmetryMethod)
                self.set_sym_state()
                return
            for part3 in part.sym:
                for part4 in part.sym:
                    if not (part3 is part4) and part4 not in part3.sym:
                        part3.sym.append(part4)
            for part4 in list(part.sym):
                self.update_part_and_children(part, part4, self.symmetryMethod)
        self.update_symmetry(part, self.symmetryMode, attach.potentialParent, attach.callerNode)
        if part.symMethod != self.symmetryMethod:
            self.symmetryMethodTmp = self.symmetryMethod
            self.symmetryModeTmp = self.symmetryMode
            self.symmetryMethod = part.symMethod
            self.tmpSymMethodInUse = True
        num = 0
        while num < self.symmetryMode:
            if vexact(part.sym[num].pos, part.pos):
                break
            num += 1
        else:
            return
        self.delete_symmetry_parts()

    def will_be_copied(self, part, as_sym):
        pass   # Part.OnWillBeCopied: modules only (no module data changes in the modelled fields)

    def on_copy(self, new_part, original, as_sym):
        pass   # Part.OnCopy: isClone / originalPart / resources: outside the modelled fields

    def update_part_and_children(self, src, upd, sym_method):
        if src is None or upd is None or src is upd:
            return
        if upd not in src.sym:
            src.sym.append(upd)
        count = len(src.sym)
        for i in range(count):
            part = src.sym[i]
            part.sym = []
            for j in range(i + 1, count):
                part.sym.append(src.sym[j])
            part.sym.append(src)
            for k in range(i):
                part.sym.append(src.sym[k])
        if src.symMethod == RADIAL and len(src.sym) > 1:
            sym_method = RADIAL
        src.symMethod = sym_method
        upd.symMethod = sym_method
        for l in range(len(upd.children)):
            self.update_part_and_children(src.children[l], upd.children[l], sym_method)

    def fire_sym_method_change(self, m):
        self.ge("onEditorSymmetryMethodChange", m)
        self.on_sym_method_change(m)

    def on_sym_method_change(self, method):
        if method == MIRROR:
            self.symmetryMode = int(min(max(float(self.symmetryMode), 0.0), 1.0))
        self.set_sym_state()

    def update_symmetry(self, sel, sym_mode, part_parent, sel_node):
        num = 1
        for c in reversed(sel.sym):
            if c.potentialParent is sel.potentialParent:
                num += 1
        root = self.rootPart
        for i in range(sym_mode):
            ref = self.ref_t(sel)
            num2 = (i + 1) % num
            if sel.symMethod == MIRROR:
                part = self.first_non_symmetrical_parent(part_parent)
                v2 = vsub(ref["pos"], part.pos)
                v3_ = project_on_plane(v2, root.up())
                newpos = vadd(vadd(part.pos, vsub(v2, v3_)), qmul_vec(angle_axis(F(180.0), vneg(root.forward())), v3_))
                d = E.t_inv_dir(root.T(), E.t_up(ref))
                d2 = E.t_inv_dir(root.T(), E.t_forward(ref))
                d = (F(-float(d[0])), d[1], d[2])
                d2 = (F(-float(d2[0])), d2[1], d2[2])
                d = E.t_dir(root.T(), d)
                d2 = E.t_dir(root.T(), d2)
                q = look_rotation(d2, d)
                # OnWillBeMirrored: ModuleMirrorPlacement only (not in the part palette)
                if sel_node is not None and sel_node.type == "Surface" and float(sel_node.orientation[0]) != 0.0:
                    q = qmul(angle_axis(F(180.0), d), q)
                self.set_symmetry_values(sel.sym[i], newpos, q)
                self.update_child_mirror(sel, part, i)
            else:
                ci = sel.sym[i]
                if ci.potentialParent is sel.potentialParent:
                    space = self.symmetryCoordSpace
                    ang = f(float(num2) * 360.0 / float(num))
                    axis = root.up() if (space == WORLD or space != SELF) else sel.potentialParent.up()
                    q = angle_axis(ang, axis)
                    vec = vadd(qmul_vec(q, vsub(ref["pos"], sel.potentialParent.pos)), ci.potentialParent.pos)
                    q2 = qmul(q, ref["rot"])
                    self.set_symmetry_values(ci, vec, q2)
                elif ci.potentialParent is not None:
                    if num2 == 0:
                        vec = ci.potentialParent.tp(sel.potentialParent.itp(ref["pos"]))
                        q2 = qmul(ci.potentialParent.rot, qmul(conj(self.ref_rot(sel.potentialParent)), ref["rot"]))
                        self.set_symmetry_values(ci, vec, q2)
                    else:
                        prev = sel.sym[num2 - 1]
                        vec = ci.potentialParent.tp(prev.potentialParent.itp(prev.pos))
                        q2 = qmul(ci.potentialParent.rot, qmul(conj(prev.potentialParent.rot), prev.rot))
                        self.set_symmetry_values(ci, vec, q2)
            if sel.attachMode == SRF_ATTACH:
                sel.sym[i].srf.srf_mesh = sel.srf.srf_mesh

    def set_symmetry_values(self, part, pos, rot):
        part.pos = pos
        part.set_rotation(rot)

    def update_child_mirror(self, part, mirror_root, sym_idx):
        root = self.rootPart
        for part2 in part.children:
            if len(part2.sym) > sym_idx and part2.sym[sym_idx] is not None and not in_hierarchy(self.selectedPart, part2.sym[sym_idx]):
                vec = vsub(part2.pos, mirror_root.pos)
                vec2 = project_on_plane(vec, root.up())
                position = vadd(vadd(mirror_root.pos, vsub(vec, vec2)), qmul_vec(angle_axis(F(180.0), vneg(root.forward())), vec2))
                d = E.t_inv_dir(root.T(), part2.up())
                d2 = E.t_inv_dir(root.T(), part2.forward())
                d = (F(-float(d[0])), d[1], d[2])
                d2 = (F(-float(d2[0])), d2[1], d2[2])
                d = E.t_dir(root.T(), d)
                d2 = E.t_dir(root.T(), d2)
                rotation = look_rotation(d2, d)
                node = part2.find_node_by_part(part)
                if node is not None and float(node.orientation[0]) != 0.0:
                    rotation = qmul(angle_axis(F(180.0), d), rotation)
                part2.sym[sym_idx].pos = position
                part2.sym[sym_idx].set_rotation(rotation)
                self.update_child_mirror(part2, mirror_root, sym_idx)

    def ref_t(self, p):
        return p.T()

    def ref_rot(self, p):
        return p.rot

    # ---- checkAttach (EditorLogic.cs:3528) ----
    def check_attach(self, sel):
        a = Attachment()
        a.caller = sel
        a.position = sel.pos
        a.rotation = self.vesselRotation
        a.potentialParent = None
        flag = False
        flag2 = False
        if sel.rules["stack"] and self.allowNode:
            for part in list(self.ship):
                if part is sel or in_hierarchy(sel, part) or (not part.rules["allowStack"] and not part.rules["allowDock"]):
                    continue
                n1 = n2 = None
                for n3 in sel.nodes:
                    if n3.attached is not None or not n3.icon:
                        continue
                    normalized = vnormalized(sel.td(n3.orientation))
                    for n4 in part.nodes:
                        if (((not n4.icon) or n4.attached is not None) and not self.cheat_clip) or n3.type != n4.type or \
                                not self.seam_icon(part, n4, sel.tp(n3.position)):
                            continue
                        normalized2 = vnormalized(part.td(n4.orientation))
                        num = dot(normalized, normalized2)
                        if not self.nonstrict:
                            if float(num) < float(F(-0.7)):
                                n2, n1 = n4, n3
                                a.potentialParent = part
                                self.stackRotation = from_to_rotation(normalized, vneg(normalized2))
                                a.rotation = qmul(self.stackRotation, self.vesselRotation)
                                a.position = vsub(part.tp(vadd(n2.position, n2.offset)), qmul_vec(qmul(a.rotation, sel.attRot), n1.position))
                                break
                        elif not (abs(float(num)) <= float(F(0.7))):
                            n2, n1 = n4, n3
                            a.potentialParent = part
                            sign = F(1.0) if float(num) >= 0.0 else F(-1.0)
                            self.stackRotation = from_to_rotation(vmul(normalized, sign), normalized2)
                            a.rotation = qmul(self.stackRotation, self.vesselRotation)
                            a.position = vsub(part.tp(vadd(n2.position, n2.offset)), qmul_vec(qmul(a.rotation, sel.attRot), n1.position))
                            break
                if n1 is not None and n2 is not None:
                    flag = True
                    a.callerNode = n1
                    a.otherNode = n2
                    sel.attachMode = STACK
        modifier = self.modifier_held()
        if (sel.rules["srfAttach"] and self.allowSrf and not modifier) or (modifier and self.isCurrentPartFlag):
            o, d = self.seam_ray(vsub(self.inp.mouse, self.srfAttachCursorOffset))
            hit = self.seam_raycast(o, d, F(1000.0), DEFAULT_EQUIVALENT)
            if hit is not None:
                part2, hpoint, hnormal = hit
                if part2 is not None:
                    if part2 not in self.ship:
                        part2 = None
                    if part2 is not None and part2.rules["allowSrfAttach"]:
                        a.callerNode = sel.srf
                        a.otherNode = None
                        sel.attachMode = SRF_ATTACH
                        sel.srf.srf_mesh = "model"
                        a.potentialParent = part2
                        flag2 = True
                        a.rotation = qmul(look_rotation(hnormal, qmul_vec(part2.rot, UP)), look_rotation(sel.srf.orientation, UP))
                        a.position = vsub(hpoint, qmul_vec(qmul(a.rotation, sel.attRot), sel.srf.position))
                        if self.use_angle_snap:
                            vector = vsub(hpoint, part2.pos)
                            ray_o = part2.pos
                            ray_d = vnormalized(part2.up())
                            vector2 = project_on_plane(vector, ray_d)
                            forward = part2.forward()
                            num2 = dot(vector, ray_d)
                            num3 = bearing_degrees(forward, vnormalized(vector2), ray_d)
                            num4 = self.srfAttachAngleSnapFine if self.fine_held() else self.srfAttachAngleSnap
                            q = f(float(num3) / float(num4))
                            if not (float(q) - float(mathf_floor(q)) > 0.5):
                                angle = f(float(math.floor(float(q))) * float(num4))
                            else:
                                angle = f(float(math.ceil(float(q))) * float(num4))
                            vector3 = qmul_vec(angle_axis(angle, ray_d), forward)
                            vector4 = vadd(ray_o, vmul(ray_d, num2))
                            vector = vnormalized(vector)
                            dist, srf_normal = self.seam_surface(part2, ray_o, ray_d, vector, hnormal)
                            vector4 = vadd(vector4, vmul(vector3, dist))
                            if abs(float(dot(srf_normal, ray_d))) < 1.0:
                                srf_normal = qmul_vec(from_to_rotation(vnormalized(project_on_plane(srf_normal, ray_d)), vector3), srf_normal)
                            a.rotation = qmul(look_rotation(srf_normal, qmul_vec(part2.rot, UP)), look_rotation(sel.srf.orientation, UP))
                            a.position = vsub(vector4, qmul_vec(qmul(a.rotation, sel.attRot), sel.srf.position))
        a.mode = sel.attachMode
        a.collision = False
        a.possible = (flag2 or flag) and not a.collision
        return a

    nonstrict = False
    craft_paths = {}

    def modifier_held(self):
        return self.inp.key("LeftAlt") or self.inp.key("RightAlt")

    def fine_held(self):
        return self.inp.key(KB["fineTweak"])

    # ---- CheckSymPartsAttach (EditorLogic.cs:3718) ----
    def check_sym_parts_attach(self, o):
        sel = self.selectedPart
        arr = []
        for part in sel.sym:
            a = Attachment()
            if o.mode == SRF_ATTACH:
                a.caller = part
                a.position = part.pos
                a.rotation = part.rot
                a.callerNode = part.srf
                a.potentialParent = part.potentialParent
                a.collision = False
                a.possible = True
                self.highlight(a.possible, part)
                arr.append(a)
            else:
                a.caller = part
                a.collision = False
                if o.callerNode is not None:
                    a.callerNode = part.find_node(o.callerNode.id)
                    node = None
                    num = F(np.finfo(np.float32).max)
                    for part2 in self.ship:
                        if not (o.potentialParent is part2) and part2 not in o.potentialParent.sym:
                            continue
                        for n2 in part2.nodes:
                            sq = sqr_mag(vsub(part.tp(o.callerNode.position), part2.tp(n2.position)))
                            if float(sq) < float(num):
                                node = n2
                                num = sq
                                a.potentialParent = part2
                    node3 = a.callerNode
                    num = F(np.finfo(np.float32).max)
                    for n2 in part.nodes:
                        sq2 = sqr_mag(vsub(a.potentialParent.tp(node.position), part.tp(n2.position)))
                        if float(sq2) < float(num):
                            num = sq2
                            node3 = n2
                    a.callerNode = node3
                    if node is not None:
                        a.otherNode = node
                        a.position = vsub(a.potentialParent.tp(node.position), qmul_vec(part.rot, node3.position))
                    a.possible = node is not None and not a.collision and node3 is not None
                    self.highlight(a.possible, part)
                    if a.possible:
                        part.pos = a.position
                    arr.append(a)
                else:
                    a.callerNode = None
                    a.possible = False
                    a.position = part.pos
                    a.rotation = part.rot
                    a.potentialParent = None
                    arr.append(a)
        if o.mode == STACK:
            for a2 in arr:
                for a3 in arr:
                    if not (a2.caller is a3.caller) and not (a2.potentialParent is not a3.potentialParent) and a2.otherNode is a3.otherNode:
                        a2.possible = False
                if not (a2.potentialParent is not o.potentialParent) and a2.otherNode is o.otherNode:
                    a2.possible = False
        return arr

    # ---- attach / detach / ship list ----
    def add_to_ship(self, part):
        self.ship.append(part)
        for p2 in find_parts_in_children(part):
            if p2 is not part:
                self.ship.append(p2)

    def remove_from_ship(self, part):
        if part in self.ship:
            self.ship.remove(part)
        for p in find_parts_in_children(part):
            if p in self.ship:
                self.ship.remove(p)

    def attach_part(self, part, attach):
        if part is self.rootPart:
            return
        part.isAttached = True
        self.add_to_ship(part)
        self.highlight(True, part, True)
        if attach.potentialParent is not None:
            self.set_parent(part, attach.potentialParent)
        if attach.callerNode is not None:
            attach.callerNode.attached = attach.potentialParent
        if attach.otherNode is not None:
            attach.otherNode.attached = part
        part.attPos0 = part.local_position()
        part.attRot0 = part.local_rotation()
        part.isAttached = True

    def detach_part(self, part):
        if not (part is self.rootPart):
            if part in self.ship:
                self.remove_from_ship(part)
            clear_attach_nodes(part, part.parent)
            self.set_parent(part, None)
            part.isAttached = False

    def attach_sym_parts(self, arr):
        for i in range(len(self.selectedPart.sym)):
            self.attach_part(self.selectedPart.sym[i], arr[i])
        return True

    def delete_part(self, part, symmetry=False, confirm=False):
        lst = find_parts_in_children(part)
        for p2 in reversed(lst):
            if p2 in self.ship:
                self.ship.remove(p2)
            p2.destroyed = True
        self.isCurrentPartFlag = False

    def delete_part_and_symmetry_parts(self, part):
        if not (part is self.selectedPart):
            self.display_icons(self.ship, self.selectedPart, False, False, False)
        self.delete_symmetry_parts()
        self.delete_part(part, False, True)

    def destroy_selected_part(self):
        sel = self.selectedPart
        if self.alive(sel):
            if sel in self.ship and sel is not self.rootPart:
                self.detach_part(sel)
                self.set_backup()
            self.fire_part_event("PartDeleted", sel)
            self.display_icons(self.ship, sel, False, False, False)
            self.delete_part_and_symmetry_parts(sel)
            self.selectedPart = None
            self.isCurrentPartFlag = False
            if len(self.ship) == 0:
                self.fsm.run_event(self.on_podDeleted)
            else:
                self.fsm.run_event(self.on_partDeleted)

    # ---- backups (ShipConstruction.CreateBackup / RestoreBackup through the craft file; world poses are saved and loaded as they are) ----
    def snapshot_ship(self):
        order = {id(p): i for i, p in enumerate(self.ship)}
        out = []
        for p in self.ship:
            out.append({
                "name": p.name, "pos": p.pos, "rot": p.rot, "attPos": p.attPos, "attPos0": p.attPos0, "attRot": p.attRot, "attRot0": p.attRot0,
                "attm": p.attachMode, "symMethod": p.symMethod,
                "children": [order[id(c)] for c in p.children if id(c) in order],
                "sym": [order[id(s)] for s in p.sym if id(s) in order],
                "nodes": [order.get(id(n.attached), -1) if n.attached is not None else -1 for n in p.nodes],
                "srf": order.get(id(p.srf.attached), -1) if p.srf.attached is not None else -1, "srfMesh": p.srf.srf_mesh,
            })
        return out

    def set_backup(self):
        if len(self.ship) != 0:
            if self.undoLevel < len(self.backups):
                del self.backups[self.undoLevel:]
            if len(self.backups) >= self.undoLimit:
                for i in range(len(self.backups) - 1):
                    self.backups[i] = self.backups[i + 1]
                self.backups[-1] = self.snapshot_ship()
            else:
                self.backups.append(self.snapshot_ship())
            self.undoLevel = len(self.backups)
            self.ge("onEditorSetBackup")
            self.ge("onEditorShipModified")

    def restore_backup(self, index):
        snap = self.backups[index]
        made = []
        for rec in snap:
            p = self.new_part(rec["name"])
            p.pos, p.rot, p.attPos, p.attPos0, p.attRot, p.attRot0 = rec["pos"], rec["rot"], rec["attPos"], rec["attPos0"], rec["attRot"], rec["attRot0"]
            p.attachMode, p.symMethod = rec["attm"], rec["symMethod"]
            p.isAttached = False
            made.append(p)
        for p, rec in zip(made, snap):
            for c in rec["children"]:
                self.set_parent(made[c], p)
                made[c].isAttached = True
            p.sym = [made[s] for s in rec["sym"]]
            for n, link in zip(p.nodes, rec["nodes"]):
                n.attached = made[link] if link >= 0 else None
            p.srf.attached = made[rec["srf"]] if rec["srf"] >= 0 else None
            p.srf.srf_mesh = rec["srfMesh"]
        return made

    def restore_state(self, offset):
        in_pod = self.fsm.current is self.st_podSelect
        if offset == 0 or len(self.backups) == 0 or (offset == -1 and len(self.ship) == 1 and len(self.backups[self.undoLevel - 1]) == 0):
            return
        num = max(1, min(self.undoLevel + offset, len(self.backups)))
        if num == self.undoLevel and not in_pod:
            return
        if self.alive(self.selectedPart):
            self.display_icons(self.ship, self.selectedPart, False, False, False)
        if len(self.ship) > 0:
            for p in find_parts_in_children(self.rootPart):
                p.destroyed = True
        self.undoLevel = num
        made = self.restore_backup(self.undoLevel - 1)
        self.ship = list(made)
        self.rootPart = made[0].local_root()
        self.ge("onEditorRestoreState")
        self.ge("onEditorShipModified")
        self.ge("onEditorUndo")
        self.fsm.run_event(self.on_undoRedo)
        if self.fsm.current is self.st_podSelect:
            self.fsm.run_event(self.on_podSelect)
        if self.fsm.started and self.alive(self.selectedPart):
            self.skipPartAttach = True
            self.coroutines.append([1, lambda: setattr(self, "skipPartAttach", False)])

    def run_coroutines(self):
        pend = self.coroutines
        self.coroutines = []
        for c in pend:
            if c[0] > 0:
                c[0] -= 1
            if c[0] <= 0:
                c[1]()
            else:
                self.coroutines.append(c)

    # ---- spawning ----
    def will_spawn(self, name):
        p = self.new_part(name)
        return p

    def on_pod_spawn(self, name):
        self.rootPart = self.new_part(name)
        self.fire_part_event("PartCreated", self.rootPart)
        self.fsm.run_event(self.on_podSelect)

    def spawn_part(self, name):
        if len(self.ship) == 0:
            self.on_pod_spawn(name)
            return
        part = self.new_part(name)
        self.selectedPart = part
        part.set_rotation(self.rootPart.rot if self.alive(self.rootPart) else part.initRot)
        pos = self.drag_over_plane()
        if pos is not None:
            part.pos = pos
        self.selPartGrabOffset = ZERO
        part.attRot = QID
        part.attRot0 = part.local_rotation()
        part.attPos0 = part.local_position()
        if self.fsm.started:
            self.fsm.run_event(self.on_partCreated)

    def part_list_tap(self, name):
        if self.fsm.current is self.st_place:
            self.part_list_background_tap()
        else:
            self.spawn_part(name)

    def part_list_background_tap(self):
        if self.fsm.current is self.st_place:
            self.destroy_selected_part()

    def release_part_to_icon(self):
        if self.fsm.started:
            self.fsm.run_event(self.on_partOverInventoryPAW)

    def freeze(self, p):
        self.frozen.add(p)
        for c in p.children:
            self.freeze(c)

    def unfreeze(self, p):
        self.frozen.discard(p)
        for c in p.children:
            self.unfreeze(c)

    # ---- camera / picking ----
    def center_drag_plane(self, pos):
        self.dragPlaneCenter = (pos[0], F(0.0), pos[2])

    def pick_part(self, mask, pick_root, pick_root_if_frozen):
        o, d = self.seam_ray(self.inp.mouse)
        hit = self.seam_raycast(o, d, F(1000.0), mask)
        if hit is None:
            return None
        part, point, _n = hit
        if part is None:
            return None
        if (pick_root_if_frozen if part in self.frozen else pick_root):
            part = part.local_root()
        cam = self.CAMERA
        normalized = vnormalized(vsub(cam, part.pos))
        normalized = (normalized[0], F(0.0), normalized[2])
        g = vsub(point, part.pos)
        look = look_rotation(vsub(normalized, vsub(normalized, d)), UP)
        g = qmul_vec(conj(look), g)
        g = (g[0], g[1], F(0.0))
        look2 = look_rotation(vsub(normalized, vsub(normalized, d)), UP)
        g = qmul_vec(look2, g)
        self.selPartGrabOffset = g
        self.center_drag_plane(vadd(part.pos, g))
        return part

    def drag_over_plane(self):
        o, d = self.seam_ray(self.inp.mouse)
        return self.drag_position(d)

    def drag_position(self, d):
        cam = self.CAMERA
        c = self.dragPlaneCenter
        num = vdist(cam, c)
        normalized = vnormalized(vsub(c, cam))
        y = f(math.atan2(float(f(float(c[0]) - float(cam[0]))), float(f(float(c[2]) - float(cam[2])))))
        y = f(float(y) * 57.29578)
        yrot = euler((F(0.0), y, F(0.0)))
        half_pi = F(1.5707963705062866)
        ac = f(math.acos(max(-1.0, min(1.0, float(dot(normalized, qmul_vec(yrot, FORWARD)))))))
        ff = f(float(half_pi) - float(ac))
        num2 = f(float(num) * float(f(math.sin(float(ff)))))
        f2 = f(math.acos(max(-1.0, min(1.0, float(dot(d, qmul_vec(yrot, FORWARD)))))))
        num3 = f(float(num2) * float(f(math.tan(float(f2)))))
        num4 = f(math.sqrt(float(f(float(num2) * float(num2) + float(num3) * float(num3)))))
        position = vsub(vadd(cam, vmul(d, num4)), self.selPartGrabOffset)
        # editorBounds without an EditorBounds instance: Bounds(zero, one * float.MaxValue): everything is inside
        return position

    # ----------------------------------------------------------------------------------------------------------------------------------------
    # EditorToolsUI (key shortcuts, SetMode, part-event reactions) and the construction mode listener
    # ----------------------------------------------------------------------------------------------------------------------------------------
    def tools_set_mode(self, mode):
        if mode == 3:
            if self.is_locked(CT["EDITOR_ROOT_REFLOW"]):
                return
        elif mode != 0:
            if self.is_locked(CT["EDITOR_GIZMO_TOOLS"]):
                return
        if mode in (1, 2) and self.alive(self.selectedPart) and self.selectedPart not in self.ship:
            self.clip("CannotPlace")
            return
        if self.toolsMode != mode:
            self.toolsMode = mode
            self.ge("onEditorConstructionModeChange", mode)
            self.on_construction_mode_changed(mode)

    def tools_update(self):
        for key, mode in (("modePlace", 0), ("modeOffset", 1), ("modeRotate", 2), ("modeRoot", 3)):
            if self.inp.key_down(KB[key]):
                self.tools_set_mode(mode)

    def tools_on_part_event(self, evt, part):
        if self.editorScreen == 3:
            return
        if evt in ("PartCreated", "PartPicked", "PartCopied", "PartRootSelected"):
            self.tools_set_mode(0)
        elif evt == "PartDeleted":
            if not self.alive(self.rootPart):
                pass
            else:
                self.tools_set_mode(0)

    def fire_part_event(self, evt, part):
        self.ge("onEditorPartEvent", evt)
        # listeners (reverse registration order; only EditorToolsUI.onEditorPartEvent is registered)
        self.tools_on_part_event(evt, part)

    def on_construction_mode_changed(self, mode):
        if mode != self.constructionMode:
            if mode == 1:
                self.fsm.run_event(self.on_goToModeOffset)
            elif mode == 2:
                self.fsm.run_event(self.on_goToModeRotate)
            elif mode == 3:
                self.fsm.run_event(self.on_goToModeRoot)
            else:
                self.fsm.run_event(self.on_goToModePlace)
            self.constructionMode = mode

    # ---- input helpers ----
    def snap_button(self):
        self.use_angle_snap = not self.use_angle_snap
        self.ge("onEditorSnapModeChange", int(self.use_angle_snap))
        if self.gizmoOffset is not None:
            self.gizmoOffset.use_grid = self.use_angle_snap
        if self.gizmoRotate is not None:
            self.gizmoRotate.use_snap = self.use_angle_snap

    def sym_input_update(self):
        if not self.is_unlocked(CT["EDITOR_SYM_SNAP_UI"]):
            return
        if self.inp.key_down(KB["toggleSymMode"]):
            self.restore_symmetry_state()
            if not self.inp.key("LeftShift") and not self.inp.key("RightShift"):
                self.next_sym_mode()
            else:
                self.prev_sym_mode()
        if self.inp.key_up(KB["toggleSymMethod"]):
            self.restore_symmetry_state()
            self.symmetryMethod = RADIAL if self.symmetryMethod == MIRROR else MIRROR
            self.fire_sym_method_change(self.symmetryMethod)
        if not self.inp.key_down(KB["coordSystem"]):
            return
        self.restore_symmetry_state()
        if self.symmetryMethod == RADIAL:
            self.symmetryCoordSpace = WORLD if self.symmetryCoordSpace == SELF else SELF
            self.ge("onEditorSymmetryCoordsChange", self.symmetryCoordSpace)

    def next_sym_mode(self):
        if self.symmetryMethod == RADIAL:
            self.symmetryMode += 1
            if self.symmetryMode == 4:
                self.symmetryMode = 5
            if self.symmetryMode == 6:
                self.symmetryMode = 7
            if self.symmetryMode >= 8:
                self.symmetryMode = 0
        else:
            self.symmetryMode += 1
            if self.symmetryMode >= 2:
                self.symmetryMode = 0
        self.ge("onEditorSymmetryModeChange", self.symmetryMode)
        self.set_sym_state()

    def prev_sym_mode(self):
        if self.symmetryMethod == RADIAL:
            self.symmetryMode -= 1
            if self.symmetryMode == 4:
                self.symmetryMode = 3
            if self.symmetryMode == 6:
                self.symmetryMode = 5
            if self.symmetryMode <= -1:
                self.symmetryMode = 7
        else:
            self.symmetryMode -= 1
            if self.symmetryMode <= -1:
                self.symmetryMode = 1
        self.ge("onEditorSymmetryModeChange", self.symmetryMode)
        self.set_sym_state()

    def snap_input_update(self):
        if self.is_unlocked(CT["EDITOR_SYM_SNAP_UI"]) and self.inp.key_down(KB["toggleAngleSnap"]):
            self.snap_button()

    def undo_redo_input_update(self):
        if not self.is_unlocked(CT["EDITOR_UNDO_REDO"]):
            return
        if self.inp.key("LeftControl"):
            if self.inp.key_down("Z"):
                self.restore_state(-1)
        if not self.inp.key("LeftControl"):
            return
        if self.inp.key_down("Y"):
            self.restore_state(1)

    def delete_input_update(self):
        flag = self.constructionMode == 0 or (self.selectedPart is not self.rootPart and self.constructionMode in (1, 2))
        if self.is_unlocked(CT["EDITOR_PAD_PICK_PLACE"]) and self.alive(self.selectedPart) and flag and self.inp.key_down("Delete"):
            self.destroy_selected_part()

    def part_rotation_input_update(self):
        self.partTweaked = False
        if self.editorScreen == 3 or self.is_unlocked(CT["EDITOR_GIZMO_TOOLS"]):
            sel = self.selectedPart
            fine = self.inp.key(KB["fineTweak"])
            ii = conj(sel.initRot)

            def rot(deg, axis):
                sel.attRot = qmul(angle_axis(F(deg), qmul_vec(ii, axis)), sel.attRot)
                self.partTweaked = True
            if self.fsm.cur_name != "st_rotate_tweak":
                if self.inp.key_down(KB["yawLeft"]):
                    rot(5 if fine else 90, FORWARD)
                elif self.inp.key_down(KB["yawRight"]):
                    rot(-5 if fine else -90, FORWARD)
                if self.inp.key_down(KB["rollLeft"]):
                    rot(5 if fine else 90, UP)
                elif self.inp.key_down(KB["rollRight"]):
                    rot(-5 if fine else -90, UP)
                elif self.inp.key_down(KB["pitchUp"]):
                    rot(-5 if fine else -90, RIGHT)
                elif self.inp.key_down(KB["pitchDown"]):
                    rot(5 if fine else 90, RIGHT)
            self.part_rotation_reset_update()
        if self.partTweaked:
            self.fire_part_event("PartRotated", self.selectedPart)
            self.partTweaked = False

    def part_rotation_reset_update(self):
        sel = self.selectedPart
        if not self.inp.key_down(KB["resetRotation"]):
            return
        self.srfAttachCursorOffset = ZERO
        in_rot = self.fsm.current is self.st_rotate_tweak
        if in_rot:
            sel.attRot = self.gizmoAttRot
            sel.set_local_rotation(self.gizmoAttRot0)
            sel.attRot0 = self.gizmoAttRot0
            if self.symUpdateNode is not None:
                sel.pos = vadd(sel.pos, vsub(self.gizmoPivot, sel.tp(self.symUpdateNode.position)))
                self.gizmoRotate.pos = sel.pos
        else:
            sel.attRot = QID
            sel.set_local_rotation(sel.attRot0)
        if self.symUpdateMode != 0:
            self.update_symmetry(sel, self.symUpdateMode, self.symUpdateParent, self.symUpdateNode)
        if self.fsm.current is self.st_rotate_tweak:
            self.fsm.run_event(self.on_rotateReset)
        self.partTweaked = True

    def part_offset_input_update(self):
        sel = self.selectedPart
        if self.is_unlocked(CT["EDITOR_GIZMO_TOOLS"]) and self.inp.key_down(KB["resetRotation"]):
            sel.attPos = ZERO
            sel.set_local_position(sel.attPos0)
            self.srfAttachCursorOffset = ZERO
            if self.symUpdateMode != 0:
                self.update_symmetry(sel, self.symUpdateMode, self.symUpdateParent, self.symUpdateNode)
            if self.fsm.current is self.st_offset_tweak:
                self.fsm.run_event(self.on_offsetReset)
            self.fire_part_event("PartOffset", sel)
        for part in sel.sym:
            part.attPos = vsub(part.local_position(), part.attPos0)

    # ---- gizmo callbacks (EditorLogic.cs:2445-2570) ----
    def on_rotate_gizmo_update(self, d_rot):
        sel = self.selectedPart
        g = self.gizmoRotate
        if g.space == SELF:
            sel.set_rotation(qmul(g.rot, sel.initRot))
        else:
            sel.set_rotation(qmul(d_rot, g.host_rot0))
        sel.attRot0 = sel.local_rotation()
        sel.attRot = qmul(qmul(self.gizmoAttRot, conj(self.gizmoAttRot0)), sel.local_rotation())
        if self.symUpdateNode is not None:
            sel.pos = vadd(sel.pos, vsub(self.gizmoPivot, sel.tp(self.symUpdateNode.position)))
            g.pos = self.gizmoPivot
        if self.symUpdateMode != 0:
            self.update_symmetry(sel, self.symUpdateMode, self.symUpdateParent, self.symUpdateNode)
        self.fire_part_event("PartRotating", sel)

    def on_rotate_gizmo_updated(self, d_rot):
        if self.selectedPart in self.ship:
            self.set_backup()
        self.fire_part_event("PartRotated", self.selectedPart)

    def on_offset_gizmo_update(self, d_pos):
        sel = self.selectedPart
        g = self.gizmoOffset
        sel.pos = g.pos
        if sel is not self.rootPart:
            pass   # TestPartBoundsSeparate needs part bounds (renderers / colliders): no bounds in the part model -> no separation
        local = sel.local_position()
        sel.attPos = vsub(local, sel.attPos0)
        if self.symUpdateMode != 0:
            self.update_symmetry(sel, self.symUpdateMode, self.symUpdateParent, self.symUpdateNode)
        self.fire_part_event("PartOffsetting", sel)

    def on_offset_gizmo_updated(self, d_pos):
        if self.selectedPart in self.ship:
            self.set_backup()
        self.fire_part_event("PartOffset", self.selectedPart)

    # ---- gizmo objects (GizmoOffset.cs / GizmoRotate.cs, Self and World spaces, no angle snap grid unless toggled) ----
    def attach_offset_gizmo(self, part):
        g = Gizmo()
        g.pos = part.pos
        g.space = self.offset_space
        g.rot = normalize_safe(qmul(part.rot, conj(part.initRot))) if g.space == SELF else QID
        g.use_grid = self.use_angle_snap
        g.dragging = False
        g.offset0 = ZERO
        g.trf0 = ZERO
        g.offset = ZERO
        self.gizmoOffset = g

    offset_space = WORLD     # GizmoOffset.coordSpace is a static, default World
    rotate_space = SELF      # GizmoRotate.coordSpace is a static, default Self

    def attach_rotate_gizmo(self, part, pivot):
        g = Gizmo()
        g.pos = pivot
        g.space = self.rotate_space
        g.host_rot0 = part.rot
        g.rot = normalize_safe(qmul(part.rot, conj(part.initRot))) if g.space == SELF else QID
        g.rot0 = g.rot
        g.use_snap = self.use_angle_snap
        g.dragging = False
        self.gizmoRotate = g

    def offset_start(self):
        g = self.gizmoOffset
        g.offset0 = ZERO
        g.trf0 = g.pos
        g.dragging = True

    def offset_move(self, axis, distance):
        g = self.gizmoOffset
        g.offset = vadd(g.offset0, vmul(axis, distance))
        g.pos = vadd(g.trf0, g.offset)
        self.on_offset_gizmo_update(g.offset)

    def offset_end(self):
        g = self.gizmoOffset
        g.dragging = False
        self.on_offset_gizmo_updated(g.offset)

    def rotate_start(self):
        g = self.gizmoRotate
        g.rot0 = g.rot
        g.dragging = True
        g.host_rot0 = self.selectedPart.rot

    def rotate_move(self, axis, d_angle):
        g = self.gizmoRotate
        if g.space == SELF:
            g.rot = normalize_safe(qmul(angle_axis(d_angle, axis), g.rot0))
        self.on_rotate_gizmo_update(angle_axis(d_angle, axis))

    def rotate_end(self, axis, delta):
        g = self.gizmoRotate
        g.dragging = False
        self.on_rotate_gizmo_updated(angle_axis(delta, axis))

    # ---- OnNewRootSelect (EditorLogic.cs:2602) ----
    def on_new_root_select(self, new_root):
        if self.make_root(new_root, self.selectedPart):
            self.clip("PartGrab")
            self.selectedPart = new_root
            if new_root in self.ship:
                self.rootPart = new_root
            self.pick_part(DEFAULT_EQUIVALENT | 4 | 0x200000, True, True)
            self.fsm.run_event(self.on_rootSelect)
        else:
            self.clip("CannotPlace")

    def root_candidates_of(self, parts):
        out = list(parts)
        count = len(out)
        while count > 0:
            count -= 1
            if not self.is_root_candidate(out[count]):
                del out[count]
        return out

    def is_root_candidate(self, p):
        # EditorReRootUtil.isRootCandidate
        if not (p is p.local_root()) and not (p is self.selectedPart):
            if not p.allowRoot:
                return False
            if not p.rules["allowSrfAttach"] and not p.rules["allowStack"]:
                return False
            if p.rules["srfAttach"] and p.srf is not None and p.srf.attached is None:
                return True
            if p.rules["stack"]:
                if p in self.ship:
                    return len(p.nodes) > 0
                count = len(p.nodes)
                while True:
                    c = count
                    count -= 1
                    if c <= 0:
                        return False
                    if p.nodes[count].attached is None:
                        return True
            return False
        return False

    @staticmethod
    def depth(p):
        n = 0
        q = p.parent
        while q is not None:
            n += 1
            q = q.parent
        return n

    def make_root(self, new_root, old_root):
        if not self.is_root_candidate(new_root):
            return False
        if not (self.depth(new_root) > self.depth(old_root)):
            return False
        self.set_hierarchy_root(new_root, new_root)
        self.strip_invalid_symmetry_sets(new_root)
        return True

    def set_hierarchy_root(self, part_self, root):
        part = part_self.parent
        flag = part_self.srf.attached is part
        part_self.parent = None if root is part_self else root
        if part is not None:
            part_self.children.append(part)
            if part_self in part.children:
                part.children.remove(part_self)
            self.set_hierarchy_root(part, part_self)
            if flag and part_self.srf.type == "Surface":
                self.reverse_srf_node_direction(part.srf, part_self.srf)
                part.attachMode = SRF_ATTACH
            else:
                part.attachMode = STACK
            self.change_srf_node_position(part_self.srf)

    def reverse_srf_node_direction(self, node, from_node):
        if node.owner is None or from_node.owner is None:
            return
        from_node.attached = None
        node.attached = from_node.owner
        vector = from_node.owner.tp(from_node.position)
        node.position = node.owner.itp(vector)

    def change_srf_node_position(self, node):
        eps = 0.001
        if node.attached is not None or node.owner is None:
            return
        owner = node.owner
        count = len(owner.nodes)
        count2 = len(owner.children)
        array = [False] * count
        num = -1
        for i in range(count):
            an = owner.nodes[i]
            array[i] = an.attached is None
            if float(sqr_mag(vsub(node.position, an.position))) < float(F(eps)):
                num = i
            if not array[i]:
                continue
            for j in range(count2):
                part = owner.children[j]
                srf = part.srf
                if srf.attached is owner and not (float(sqr_mag(vsub(owner.tp(an.position), part.tp(srf.position)))) >= float(F(eps))):
                    array[i] = False
                    break
        if num > 0 and array[num]:
            return
        num2 = 0
        while True:
            if num2 < count:
                if array[num2]:
                    break
                num2 += 1
                continue
            node.position = ZERO
            node.orientation = node.orig_orientation
            return
        node.position = owner.nodes[num2].position
        node.orientation = owner.nodes[num2].orientation

    def strip_invalid_symmetry_sets(self, p):
        if not self.symmetry_set_is_valid(p):
            for c in reversed(p.sym):
                c.sym = []
            p.sym = []
        for c in reversed(list(p.children)):
            self.strip_invalid_symmetry_sets(c)

    def symmetry_set_is_valid(self, p):
        num = len(p.sym) - 1
        if num >= 0:
            d = self.depth(p)
            n2 = num
            while n2 >= 0:
                if self.depth(p.sym[n2]) == d:
                    n2 -= 1
                    continue
                return False
        return True

    # ----------------------------------------------------------------------------------------------------------------------------------------
    # SetupFSM (EditorLogic.cs:1236-2365)
    # ----------------------------------------------------------------------------------------------------------------------------------------
    def setup_fsm(self):
        S = KState
        self.st_podSelect, self.st_idle, self.st_place = S("st_podSelect"), S("st_idle"), S("st_place")
        self.st_offset_select, self.st_offset_tweak = S("st_offset_select"), S("st_offset_tweak")
        self.st_rotate_select, self.st_rotate_tweak = S("st_rotate_select"), S("st_rotate_tweak")
        self.st_root_unselected, self.st_root_select = S("st_root_unselected"), S("st_root_select")
        E_ = KEvent
        for n in ("on_podSelect on_podDeleted on_partCreated on_partPicked on_partCopied on_partReveal on_partDropped on_partAttached "
                  "on_partDeleted on_partLost on_partOverInventoryPAW on_goToModeOffset on_offsetSelect on_offsetDeselect on_offsetReset "
                  "on_goToModeRotate on_rotateSelect on_rotateDeselect on_rotateReset on_goToModeRoot on_rootPickSet on_rootDeselect "
                  "on_rootSelect on_rootSelectFail on_goToModePlace on_undoRedo on_newShip on_shipLoaded").split():
            setattr(self, n, E_(n))
        fsm = self.fsm
        sp, si, sl = self.st_podSelect, self.st_idle, self.st_place
        so_s, so_t, sr_s, sr_t = self.st_offset_select, self.st_offset_tweak, self.st_rotate_select, self.st_rotate_tweak
        su, ssel = self.st_root_unselected, self.st_root_select
        MASKLOCK = (CT["UI"] | CT["EDITOR_ICON_HOVER"] | CT["EDITOR_ICON_PICK"] | CT["EDITOR_TAB_SWITCH"] | CT["EDITOR_LOAD"] | CT["EDITOR_EXIT"] |
                    CT["EDITOR_UNDO_REDO"] | CT["GUI"])

        # st_podSelect
        def pod_enter(frm):
            if frm is sp:
                pass
            else:
                self.editorScreen = 0
                self.set_lock("EditorLogic_rootPartMode", (~MASKLOCK) & 0xFFFFFFFFFFFFFFFF)
        sp.on_enter = pod_enter
        sp.on_update = lambda: (self.undo_redo_input_update_pod())

        def pod_leave(to):
            if to is si:
                if fsm.last_event is self.on_podSelect and self.alive(self.rootPart) and self.rootPart not in self.ship:
                    rp = self.rootPart
                    rp.pos = self.initial_pod_position()
                    rp.attPos0 = self.initial_pod_position()
                    rp.set_rotation(self.vesselRotation)
                    rp.attRot0 = self.vesselRotation
                    self.restore_symmetry_state()
                    rp.symMethod = self.symmetryMethod
                    self.add_to_ship(rp)
                    self.set_backup()
                    self.ge("onEditorPodPicked")
                    self.clip("Attach")
                self.remove_lock("EditorLogic_rootPartMode")
        sp.on_leave = pod_leave
        fsm.add_state(sp)

        # st_idle
        si.on_update = lambda: (self.sym_input_update(), self.snap_input_update(), self.undo_redo_input_update())
        fsm.add_state(si)

        # st_place
        def place_enter(frm):
            self.delete_symmetry_parts()
            self.restore_symmetry_state()
            self.clip("PartGrab")
            if self.selectedPart is self.rootPart:
                self.ge("onEditorPodSelected")
        sl.on_enter = place_enter

        def place_update():
            self.sym_input_update()
            self.snap_input_update()
            self.undo_redo_input_update()
            if self.inp.right.up and self.inp.right.was_dragging():
                self.srfAttachCursorOffset = ZERO
            if self.alive(self.selectedPart):
                sel = self.selectedPart
                position = self.drag_over_plane()
                if position is not None:
                    sel.pos = vlerp(sel.pos, position, f(float(self.dragSharpness) * float(self.clock.dt)))
                sel.set_rotation(qmul(qmul(self.vesselRotation, sel.initRot), sel.attRot))
                if not self.skipPartAttach:
                    self.attachment = self.check_attach(sel)
                elif self.attachment is not None:
                    self.attachment.possible = False
                at = self.attachment
                sel.attRot0 = at.rotation
                sel.attPos0 = at.position
                if at.possible:
                    sel.set_rotation(qmul(at.rotation, sel.attRot))
                sel.pos = at.position
                self.highlight(at.possible, sel)
                if sel.potentialParent is not None and at.potentialParent is not sel.potentialParent:
                    self.delete_symmetry_parts()
                    self.wipe_symmetry(sel)
                sel.potentialParent = at.potentialParent
                if at.potentialParent is not None:
                    self.create_symmetry(self.symmetryMode, at)
                else:
                    self.delete_symmetry_parts()
                    if self.symmetryModeBeforeNodeAttachment >= 0:
                        self.restore_symmetry_mode_before_node()
                self.cPartAttachments = self.check_sym_parts_attach(at)
                self.display_icons(self.ship, sel, sel.rules["stack"], sel.rules["srfAttach"], sel.rules["allowDock"])
                self.fire_part_event("PartDragging", sel)
            self.part_rotation_input_update()
            self.delete_input_update()
        sl.on_update = place_update

        def place_leave(to):
            if self.alive(self.selectedPart):
                self.display_icons(self.ship, self.selectedPart, False, False, False)
            self.clean_symmetry()
            self.ge("onEditorPartPlaced")
            if to is si and self.alive(self.selectedPart):
                self.selectedPart = None
            if to is not sr_t:
                self.srfAttachCursorOffset = ZERO
        sl.on_leave = place_leave
        fsm.add_state(sl)

        # st_offset_select
        so_s.on_update = lambda: (self.undo_redo_input_update(), self.snap_input_update())
        fsm.add_state(so_s)

        # st_offset_tweak
        def offset_enter(frm):
            sel = self.selectedPart
            self.log("tw:start")
            self.symUpdateMode = len(sel.sym)
            if sel in self.ship:
                self.symUpdateParent = sel.parent
                self.symUpdateNode = sel.find_node_by_part(self.symUpdateParent)
            else:
                self.symUpdateParent = self.attachment.potentialParent
                self.symUpdateNode = self.attachment.callerNode
            self.attach_offset_gizmo(sel)
            self.clip("TweakGrab")
        so_t.on_enter = offset_enter

        def offset_update():
            self.part_offset_input_update()
            self.undo_redo_input_update()
            self.snap_input_update()
            self.delete_input_update()
            if self.inp.key_up(KB["coordSystem"]) and not self.gizmoOffset.dragging:
                self.set_offset_space(SELF if self.gizmoOffset.space == WORLD else WORLD)
        so_t.on_update = offset_update

        def offset_leave(to):
            self.gizmoOffset = None
            self.symUpdateMode = 0
            self.symUpdateParent = None
            self.symUpdateNode = None
            if to is not so_t and to is not sr_t and self.alive(self.selectedPart):
                self.log("tw:end")
                if to is si:
                    self.selectedPart = None
            self.clip("TweakRelease")
        so_t.on_leave = offset_leave
        fsm.add_state(so_t)

        # st_rotate_select
        sr_s.on_update = lambda: (self.undo_redo_input_update(), self.snap_input_update())
        fsm.add_state(sr_s)

        # st_rotate_tweak
        def rotate_enter(frm):
            sel = self.selectedPart
            self.log("tw:start")
            self.symUpdateMode = len(sel.sym)
            if sel in self.ship:
                self.symUpdateParent = sel.parent
                self.symUpdateNode = sel.find_node_by_part(self.symUpdateParent)
            else:
                self.symUpdateParent = self.attachment.potentialParent
                self.symUpdateNode = self.attachment.callerNode
            if self.symUpdateNode is not None:
                self.gizmoPivot = sel.tp(self.symUpdateNode.position)
            else:
                self.gizmoPivot = sel.pos
            self.attach_rotate_gizmo(sel, self.gizmoPivot)
            self.gizmoAttRot = sel.attRot
            self.gizmoAttRot0 = sel.attRot0
            self.clip("TweakGrab")
        sr_t.on_enter = rotate_enter

        def rotate_update():
            self.part_rotation_input_update()
            self.undo_redo_input_update()
            self.snap_input_update()
            self.delete_input_update()
            if self.inp.key_up(KB["coordSystem"]) and not self.gizmoRotate.dragging:
                self.set_rotate_space(SELF if self.gizmoRotate.space == WORLD else WORLD)
        sr_t.on_update = rotate_update

        def rotate_leave(to):
            if to is sl and self.selectedPart not in self.ship and self.attachment is not None and self.attachment.possible and \
                    self.attachment.mode == SRF_ATTACH:
                ws = self.world_to_screen(self.selectedPart.tp(self.attachment.callerNode.position))
                self.srfAttachCursorOffset = vsub(self.inp.mouse, ws)
            else:
                self.srfAttachCursorOffset = ZERO
            self.gizmoRotate = None
            self.symUpdateMode = 0
            self.symUpdateParent = None
            self.symUpdateNode = None
            if to is not so_t and to is not sr_t and self.alive(self.selectedPart):
                self.log("tw:end")
                if to is si:
                    self.selectedPart = None
            self.clip("TweakRelease")
        sr_t.on_leave = rotate_leave
        fsm.add_state(sr_t)

        # st_root_unselected (the C# assigns an empty OnUpdate last)
        su.on_enter = lambda frm: None
        su.on_update = lambda: None
        fsm.add_state(su)

        # st_root_select
        def root_sel_enter(frm):
            self.rootCandidates = []
            lst = self.root_candidates_of(find_parts_in_children(self.selectedPart))
            for p in lst:
                self.rootCandidates.append(p)
        ssel.on_enter = root_sel_enter
        ssel.on_update = lambda: (self.undo_redo_input_update(), self.snap_input_update())

        def root_sel_leave(to):
            self.rootCandidates = []
            if to is sl:
                self.tools_set_mode(0)
            elif to is so_t:
                self.tools_set_mode(1)
            elif to is sr_t:
                self.tools_set_mode(2)
        ssel.on_leave = root_sel_leave
        fsm.add_state(ssel)

        # ---- events ----
        pods = (self.on_podSelect, self.on_podDeleted)
        self.on_podSelect.goto = si
        self.on_podSelect.mode = MANUAL
        self.on_podSelect.check = lambda st: self.alive(self.rootPart)
        fsm.add_event(self.on_podSelect, [sp])

        self.on_podDeleted.goto = sp
        self.on_podDeleted.mode = MANUAL
        self.on_podDeleted.check = lambda st: not self.alive(self.rootPart)

        def pod_deleted():
            self.set_backup()
            if self.editorScreen == 3:
                self.editorScreen = 0
            self.ge("onEditorPodDeleted")
        self.on_podDeleted.on_event = pod_deleted
        fsm.add_event_excluding(self.on_podDeleted, [])

        self.on_partCreated.goto = sl
        self.on_partCreated.mode = MANUAL

        def part_created():
            self.clip("PartGrab")
            self.fire_part_event("PartCreated", self.selectedPart)
            self.isCurrentPartFlag = self.alive(self.selectedPart) and self.has_flag_decal(self.selectedPart)
        self.on_partCreated.on_event = part_created
        fsm.add_event_excluding(self.on_partCreated, [sl])

        # on_partPicked
        self.on_partPicked.mode = UPDATE
        self.on_partPicked.goto = sl

        def picked_check(st):
            if self.is_unlocked(CT["EDITOR_PAD_PICK_PLACE"]) and self.inp.mb_up[0] and not self.inp.left.was_dragging(25.0) and \
                    fsm.frames_in_current() > 1 and not self.pointer_over_ui() and not self.held_stacked():
                shift = self.inp.key("LeftShift")
                self.selectedPart = self.pick_part(LAYER_MASK | 0x200000, shift, not shift)
                self.isCurrentPartFlag = self.alive(self.selectedPart) and self.has_flag_decal(self.selectedPart)
                if self.alive(self.selectedPart) and not self.modifier_held():
                    return not self.inp.key("LeftControl")
                return False
            return False
        self.on_partPicked.check = picked_check
        self.on_partPicked.on_event = self.part_picked_event
        fsm.add_event(self.on_partPicked, [si])

        self.on_partCopied.goto = sl
        self.on_partCopied.mode = UPDATE

        def copied_check(st):
            if self.is_unlocked(CT["EDITOR_PAD_PICK_PLACE"]) and self.inp.mb_up[0] and not self.inp.left.was_dragging(25.0) and \
                    fsm.frames_in_current() > 1 and not self.pointer_over_ui():
                self.selectedPart = self.pick_part(LAYER_MASK | 0x200000, self.inp.key("LeftShift"), False)
                if self.alive(self.selectedPart) and self.selectedPart is not self.rootPart and self.modifier_held():
                    return not self.inp.key("LeftControl")
                return False
            return False
        self.on_partCopied.check = copied_check

        def copied_event():
            sel = self.selectedPart
            part3 = self.duplicate_part(sel)
            self.selectedPart = part3
            clear_attach_nodes(part3, part3.parent)
            self.clear_parent(part3)
            self.wipe_symmetry(part3)
            if part3 in self.frozen:
                self.unfreeze(part3)
            self.selPartGrabOffset = ZERO
            self.fire_part_event("PartCopied", part3)
        self.on_partCopied.on_event = copied_event
        fsm.add_event(self.on_partCopied, [si])

        self.on_partReveal.goto = si
        self.on_partReveal.mode = UPDATE

        def reveal_check(st):
            if self.is_unlocked(CT["EDITOR_PAD_PICK_PLACE"]) and self.inp.mb_up[0] and not self.inp.left.was_dragging(25.0) and \
                    fsm.frames_in_current() > 1 and not self.pointer_over_ui():
                self.selectedPart = self.pick_part(LAYER_MASK | 0x200000, self.inp.key("LeftShift"), False)
                if self.alive(self.selectedPart):
                    return self.inp.key("LeftControl")
                return False
            return False
        self.on_partReveal.check = reveal_check

        def reveal_event():
            self.log("reveal:" + self.selectedPart.name)
            self.selectedPart = None
        self.on_partReveal.on_event = reveal_event
        fsm.add_event(self.on_partReveal, [si])

        self.on_partDropped.mode = LATE

        def dropped_check(st):
            if self.alive(self.selectedPart) and fsm.frames_in_current() > 0 and self.inp.mb_up[0] and not self.attachment.possible and \
                    not self.modal_over() and not self.pointer_over_ui() and not self.held_stacked():
                flag2 = True
                for a in reversed(self.cPartAttachments):
                    if a.collision:
                        flag2 = False
                        break
                if not self.attachment.collision and flag2:
                    if self.selectedPart is self.rootPart:
                        self.set_backup()
                    return True
                self.clip("CannotPlace")
                return False
            return False
        self.on_partDropped.check = dropped_check

        def dropped_event():
            self.clip("PartRelease")
            self.isCurrentPartFlag = False
            self.delete_symmetry_parts()
            if self.selectedPart is not self.rootPart:
                self.freeze(self.selectedPart)
            self.fire_part_event("PartDropped", self.selectedPart)
            self.on_partDropped.goto = si
        self.on_partDropped.on_event = dropped_event
        fsm.add_event(self.on_partDropped, [sl])

        self.on_partAttached.mode = UPDATE
        self.on_partAttached.check = lambda st: self.alive(self.selectedPart) and self.inp.mb_up[0] and fsm.frames_in_current() > 0 and \
            self.attachment.possible and not self.pointer_over_ui()

        def attached_event():
            self.isCurrentPartFlag = False
            sel = self.selectedPart
            if len(sel.sym) > 0:
                self.restore_symmetry_state()
                flag = True
                for a in reversed(self.cPartAttachments):
                    if not a.possible:
                        flag = False
                        break
                if not flag:
                    self.clip("CannotPlace")
                    self.on_partAttached.goto = sl
                    return
                self.attach_part(sel, self.attachment)
                self.attach_sym_parts(self.cPartAttachments)
            else:
                self.attach_part(sel, self.attachment)
                if self.symmetryModeBeforeNodeAttachment >= 0:
                    self.restore_symmetry_mode_before_node()
            self.set_backup()
            self.clip("Attach")
            self.on_partAttached.goto = si
            self.center_drag_plane(vadd(sel.pos, self.selPartGrabOffset))
            self.fire_part_event("PartAttached", sel)
        self.on_partAttached.on_event = attached_event
        fsm.add_event(self.on_partAttached, [sl])

        self.on_partDeleted.goto = si
        self.on_partDeleted.mode = MANUAL

        def deleted_event():
            if self.alive(self.selectedPart):
                self.destroy_selected_part()
            self.isCurrentPartFlag = False
            self.ge("onEditorPartDeleted")
        self.on_partDeleted.on_event = deleted_event
        fsm.add_event(self.on_partDeleted, [sl, so_t, sr_t])

        self.on_partLost.goto = si
        self.on_partLost.mode = UPDATE
        self.on_partLost.check = lambda st: not self.alive(self.selectedPart)

        def lost_event():
            self.isCurrentPartFlag = False
            self.fire_part_event("Unknown", self.selectedPart)
        self.on_partLost.on_event = lost_event
        fsm.add_event(self.on_partLost, [sl])

        self.on_partOverInventoryPAW.goto = si
        self.on_partOverInventoryPAW.mode = MANUAL

        def inv_event():
            if not self.alive(self.selectedPart):
                return
            self.isCurrentPartFlag = False
            self.delete_symmetry_parts()
            if self.selectedPart is not self.rootPart:
                self.freeze(self.selectedPart)
            self.fire_part_event("PartOverInventoryGrid", self.selectedPart)
        self.on_partOverInventoryPAW.on_event = inv_event
        fsm.add_event_excluding(self.on_partOverInventoryPAW, [])

        self.on_goToModeOffset.mode = MANUAL

        def goto_offset():
            if not self.alive(self.selectedPart):
                self.on_goToModeOffset.goto = so_s
            elif self.selectedPart not in self.ship:
                self.on_goToModeOffset.goto = sl
                self.on_partPicked.on_event()
            else:
                self.on_goToModeOffset.goto = so_t
        self.on_goToModeOffset.on_event = goto_offset
        fsm.add_event(self.on_goToModeOffset, [si, sr_s, sr_t, su, ssel])

        self.on_offsetSelect.mode = UPDATE

        def offset_select_check(st):
            if self.inp.mb_up[0] and not self.pointer_over_ui():
                self.selectedPart = self.pick_part(LAYER_MASK | 4 | 0x200000, self.inp.key("LeftShift"), False)
                if self.alive(self.selectedPart):
                    if self.selectedPart not in self.ship:
                        self.on_offsetSelect.goto = sl
                        self.on_partPicked.on_event()
                        return False
                    self.on_offsetSelect.goto = so_t
                    return True
            return False
        self.on_offsetSelect.check = offset_select_check
        fsm.add_event(self.on_offsetSelect, [so_s])

        self.on_offsetDeselect.goto = so_s
        self.on_offsetDeselect.mode = UPDATE

        def offset_deselect_check(st):
            if self.inp.left.down and not self.inp.left.was_dragging() and not False and not self.pointer_over_ui():
                part2 = self.pick_part(LAYER_MASK | 4 | 0x200000, self.inp.key("LeftShift"), False)
                if not self.alive(part2):
                    self.log("tw:end")
                    self.selectedPart = None
                    return True
                ws = self.world_to_screen(self.gizmoOffset.pos)
                if float(vdist(ws, self.inp.mouse)) > 75.0:
                    self.log("tw:end")
                    self.selectedPart = part2
                    return True
            return False
        self.on_offsetDeselect.check = offset_deselect_check
        fsm.add_event(self.on_offsetDeselect, [so_t])

        self.on_offsetReset.goto = so_t
        self.on_offsetReset.mode = MANUAL
        fsm.add_event(self.on_offsetReset, [so_t])

        self.on_goToModeRotate.mode = MANUAL

        def goto_rotate():
            if not self.alive(self.selectedPart):
                self.on_goToModeRotate.goto = sr_s
            elif self.selectedPart not in self.ship:
                self.on_goToModeRotate.goto = sl
                self.on_partPicked.on_event()
            else:
                self.on_goToModeRotate.goto = sr_t
        self.on_goToModeRotate.on_event = goto_rotate
        fsm.add_event(self.on_goToModeRotate, [si, so_s, so_t, su, ssel])

        self.on_rotateSelect.goto = sr_t
        self.on_rotateSelect.mode = UPDATE

        def rotate_select_check(st):
            if self.inp.mb_up[0] and not self.pointer_over_ui():
                self.selectedPart = self.pick_part(LAYER_MASK | 4 | 0x200000, self.inp.key("LeftShift"), False)
                if self.alive(self.selectedPart):
                    if self.selectedPart not in self.ship:
                        self.on_rotateSelect.goto = sl
                        self.on_partPicked.on_event()
                        return False
                    self.on_rotateSelect.goto = sr_t
                    return True
            return False
        self.on_rotateSelect.check = rotate_select_check
        fsm.add_event(self.on_rotateSelect, [sr_s])

        self.on_rotateDeselect.goto = sr_s
        self.on_rotateDeselect.mode = UPDATE

        def rotate_deselect_check(st):
            if self.inp.left.down and not self.inp.left.was_dragging() and not False and not self.pointer_over_ui():
                part = self.pick_part(LAYER_MASK | 4 | 0x200000, self.inp.key("LeftShift"), False)
                if not self.alive(part):
                    self.log("tw:end")
                    self.selectedPart = None
                    return True
                ws = self.world_to_screen(self.gizmoRotate.pos)
                if float(vdist(ws, self.inp.mouse)) > 75.0:
                    self.log("tw:end")
                    self.selectedPart = part
                    return True
            return False
        self.on_rotateDeselect.check = rotate_deselect_check
        fsm.add_event(self.on_rotateDeselect, [sr_t])

        self.on_rotateReset.goto = sr_t
        self.on_rotateReset.mode = MANUAL
        fsm.add_event(self.on_rotateReset, [sr_t])

        self.on_goToModeRoot.mode = MANUAL

        def goto_root():
            if self.alive(self.selectedPart) and fsm.current in (sl, so_t, sr_t):
                self.on_goToModeRoot.goto = ssel
            else:
                self.on_goToModeRoot.goto = su
        self.on_goToModeRoot.on_event = goto_root
        fsm.add_event_excluding(self.on_goToModeRoot, [ssel, su, sp])

        self.on_rootPickSet.mode = UPDATE

        def root_pick_check(st):
            if self.inp.mb_up[0] and not self.inp.left.was_dragging(25.0) and not self.pointer_over_ui():
                self.selectedPart = self.pick_part(LAYER_MASK | 4 | 0x200000, True, True)
                return self.alive(self.selectedPart)
            return False
        self.on_rootPickSet.check = root_pick_check

        def root_pick_event():
            if len(self.selectedPart.children) > 0:
                self.on_rootPickSet.goto = ssel
            else:
                self.on_rootPickSet.goto = su
                self.clip("CannotPlace")
                self.selectedPart = None
        self.on_rootPickSet.on_event = root_pick_event
        fsm.add_event(self.on_rootPickSet, [su])

        self.on_rootDeselect.goto = su
        self.on_rootDeselect.mode = LATE

        def root_deselect_check(st):
            if self.inp.left.up and not self.inp.left.was_dragging() and not self.alive(self.pick_part(LAYER_MASK | 4 | 0x200000, False, False)):
                self.selectedPart = None
                return True
            return False
        self.on_rootDeselect.check = root_deselect_check
        fsm.add_event(self.on_rootDeselect, [ssel])

        self.on_rootSelect.goto = sl
        self.on_rootSelect.mode = MANUAL

        def root_select_event():
            self.clip("ReRoot")
            self.fire_part_event("PartRootSelected", self.selectedPart)
        self.on_rootSelect.on_event = root_select_event
        fsm.add_event(self.on_rootSelect, [ssel])

        self.on_rootSelectFail.mode = UPDATE
        self.on_rootSelectFail.goto = su
        self.on_rootSelectFail.check = lambda st: fsm.frames_in_current() > 0 and len(self.rootCandidates) == 0

        def root_fail_event():
            self.clip("CannotPlace")
            self.selectedPart = None
        self.on_rootSelectFail.on_event = root_fail_event
        fsm.add_event(self.on_rootSelectFail, [ssel])

        self.on_goToModePlace.mode = MANUAL

        def goto_place():
            if not (not self.alive(self.selectedPart)) and self.selectedPart not in self.ship:
                self.unfreeze(self.selectedPart)
                self.on_goToModePlace.goto = sl
            else:
                self.on_goToModePlace.goto = si
        self.on_goToModePlace.on_event = goto_place
        fsm.add_event(self.on_goToModePlace, [so_s, so_t, sr_s, sr_t, su, ssel])

        self.on_undoRedo.mode = MANUAL

        def undo_event():
            if fsm.current is sr_t:
                self.on_undoRedo.goto = sr_s
            elif fsm.current is so_t:
                self.on_undoRedo.goto = so_s
        self.on_undoRedo.on_event = undo_event
        fsm.add_event(self.on_undoRedo, [so_t, sr_t])

        self.on_newShip.goto = sp
        self.on_newShip.mode = MANUAL

        def new_ship_event():
            for p in list(self.ship):
                p.destroyed = True
            for p in list(self.parts):
                p.destroyed = True
            self.selectedPart = None
        self.on_newShip.on_event = new_ship_event
        fsm.add_event_excluding(self.on_newShip, [])

        self.on_shipLoaded.goto = si
        self.on_shipLoaded.mode = MANUAL

        def ship_loaded_event():
            for p in list(self.parts):
                if p not in self.ship:
                    p.destroyed = True
            self.selectedPart = None
        self.on_shipLoaded.on_event = ship_loaded_event
        fsm.add_event_excluding(self.on_shipLoaded, [])

    # ---- the pieces of the states that need access to self ----
    def undo_redo_input_update_pod(self):
        self.undo_redo_input_update()

    ui_over = False

    def pointer_over_ui(self):
        return self.ui_over

    def held_stacked(self):
        return False

    def modal_over(self):
        return False

    def initial_pod_position(self):
        return self.pod_position

    pod_position = v3(0.0, 5.0, 0.0)   # EditorLogic.initialPodPosition without an EditorBounds instance (EditorLogic.cs:376)

    def set_offset_space(self, space):
        g = self.gizmoOffset
        if space != g.space:
            if space == SELF:
                g.rot = normalize_safe(qmul(self.selectedPart.rot, conj(self.selectedPart.initRot)))
            else:
                g.rot = QID
            g.offset0 = ZERO
            g.trf0 = g.pos
            g.space = space
            Sim.offset_space = space

    def set_rotate_space(self, space):
        g = self.gizmoRotate
        if space != g.space:
            if space == SELF:
                g.rot = normalize_safe(qmul(self.selectedPart.rot, conj(self.selectedPart.initRot)))
            else:
                g.rot = QID
            g.rot0 = g.rot
            g.space = space
            Sim.rotate_space = space

    def part_picked_event(self):
        sel = self.selectedPart
        if sel is not sel.local_root():
            num5 = sel in self.ship
            self.detach_part(sel)
            self.delete_symmetry_parts()
            if num5:
                self.ge("onEditorPartPicked")
                self.set_backup()
                self.fire_part_event("PartDetached", sel)
                return
        else:
            self.set_backup()
        if sel in self.frozen:
            self.unfreeze(sel)
        self.isCurrentPartFlag = self.alive(sel) and self.has_flag_decal(sel)
        self.fire_part_event("PartPicked", sel)


    # ----------------------------------------------------------------------------------------------------------------------------------------
    # lifetime: Start, StartEditor, NewShip, load; frame driver
    # ----------------------------------------------------------------------------------------------------------------------------------------
    def start(self):
        self.on_sym_method_change(self.symmetryMethod)
        self.start_editor(False, "clean", None)
        if self.alive(self.rootPart):
            self.fsm.start(self.st_idle)
        else:
            self.fsm.start(self.st_podSelect)

    def start_editor(self, restart, behaviour, craft):
        if restart:
            self.ge("onEditorRestart")
        if not self.restarting:
            self.remove_lock("Editor_outOfPartMode")
            self.remove_lock("EditorLogic_rootPartMode")
        self.on_sym_method_change(self.symmetryMethod)
        self.editorScreen = 0
        self.undoLevel = 0
        self.undoLimit = 32
        self.backups = []
        self.ship = []
        loaded = False
        if behaviour == "file":
            made = self.load_craft(craft)
            if made is not None:
                self.ship = made
                self.rootPart = made[0].local_root()
                self.editorScreen = 0
                self.set_backup()
                loaded = True
        if not loaded:
            self.rootPart = None
        self.undoIndexAtLastSave = self.undoLevel
        if restart:
            if self.alive(self.rootPart):
                self.fsm.run_event(self.on_shipLoaded)
            else:
                self.fsm.run_event(self.on_newShip)
        self.ge("onEditorStarted")

    def new_ship(self):
        self.tools_set_mode(0)
        if self.undoIndexAtLastSave != self.undoLevel and len(self.ship) > 0:
            self.log("dialog:newShip")
        self.remove_lock("SaveConfirmationDialog")
        self.ge("onEditorNewShipDialogDismiss")
        self.start_editor(True, "clean", None)

    def ship_to_load_selected(self, craft):
        self.remove_lock("EditorLogic_dialog_softLock")
        self.start_editor(True, "file", craft)
        self.ge("onEditorLoad")

    def load_craft(self, craft):
        """ShipConstruct.LoadShip of a stock craft: parts in file order with their saved poses, links, symmetry sets and node links"""
        raw = E.parse_craft(craft)
        made = []
        by_id = {}
        for r in raw:
            name, cid = E.split_id(r["part"])
            p = self.new_part(name)
            g = E.floats
            p.pos = tuple(g(r["pos"]))
            p.rot = tuple(g(r["rot"]))
            p.attPos = tuple(g(r["attPos"]))
            p.attPos0 = tuple(g(r["attPos0"]))
            p.attRot = tuple(g(r["attRot"]))
            p.attRot0 = tuple(g(r["attRot0"]))
            p.attachMode = int(r.get("attm", "0"))
            p.symMethod = MIRROR if r.get("symMethod", "Radial") == "Mirror" else RADIAL
            made.append(p)
            by_id[cid] = p
        for p, r in zip(made, raw):
            for ln in r.get("link", []):
                c = by_id.get(E.split_id(ln)[1])
                if c is not None:
                    self.set_parent(c, p)
                    c.isAttached = True
            p.sym = [by_id[E.split_id(s)[1]] for s in r.get("sym", []) if E.split_id(s)[1] in by_id]
            for an in r.get("attN", []):
                nid, _, rest = an.partition(",")
                tgt = by_id.get(E.split_id(rest.split("_")[0] + "_" + rest.split("_")[1])[1])
                n = p.find_node(nid)
                if n is not None and tgt is not None:
                    n.attached = tgt
            sn = r.get("srfN")
            if sn:
                sv = sn[0] if isinstance(sn, list) else sn
                fields = sv.split(",")
                if len(fields) > 1 and fields[1]:
                    tgt = by_id.get(E.split_id(fields[1])[1])
                    if tgt is not None:
                        p.srf.attached = tgt
        return made

    def update(self):
        # EditorLogic.Update then EditorToolsUI.Update
        if self.fsm.started:
            self.fsm.update()
        self.tools_update()

    def late_update(self):
        if self.fsm.started:
            self.fsm.late_update()

    def snapshot_string(self):
        out = ["S|%s|%s|%s|%d|%d|%d|%d|%d|%s" % (self.fsm.cur_name, self.pid(self.selectedPart), self.pid(self.rootPart), self.undoLevel,
                                                len(self.backups), self.symmetryMode, self.symmetryMethod, self.symmetryCoordSpace,
                                                ",".join(str(p.id) for p in self.ship))]
        nvals = 0
        for p in self.parts:
            if p.destroyed:
                out.append("D|%d|%s" % (p.id, p.name))
                continue
            vals = [p.pos, p.rot, p.attPos, p.attPos0, p.attRot, p.attRot0]
            nvals += sum(len(v) for v in vals)
            nodes = ";".join("%s:%s" % (n.id, self.pid(n.attached)) for n in p.nodes)
            out.append("P|%d|%s|%s|%s|%d|%d|%s|%s|%s|%s|%s|%d|%d" % (
                p.id, p.name, self.pid(p.parent), "|".join(" ".join(hexf(c) for c in v) for v in vals), p.attachMode, p.symMethod,
                ",".join(str(c.id) for c in p.children), ",".join(str(c.id) for c in p.sym), nodes, self.pid(p.srf.attached) if p.srf else "-",
                p.srf.srf_mesh if p.srf else "", int(p in self.frozen), int(p in self.ship)))
        return "\n".join(out), nvals

    def frame(self, dt, mouse, mb, keys, cmds, ui=False):
        """one engine frame: Time, Input, host commands, Mouse.Update, EditorLogic.Update, coroutines, LateUpdate"""
        self.clock.frame += 1
        self.ui_over = ui
        self.clock.dt = F(dt)
        self.clock.time = f(float(self.clock.time) + float(dt))
        inp = self.inp
        inp.mouse = mouse
        inp.mb_down, inp.mb_up, inp.mb_held = list(mb[0]), list(mb[1]), list(mb[2])
        inp.kd, inp.ku, inp.kh = set(keys[0]), set(keys[1]), set(keys[2])
        self.trace.append("frame")
        for c in cmds:
            self.exec_cmd(c)
        inp.mouse_update()
        self.update()
        self.run_coroutines()
        self.late_update()

    def part_by_id(self, pid):
        for p in self.parts:
            if p.id == pid:
                return p
        raise KeyError(pid)

    def exec_cmd(self, c):
        k = c[0]
        self.trace.append("cmd:" + k)
        if k == "spawn":
            self.part_list_tap(c[1])
        elif k == "mode":
            self.tools_set_mode(c[1])
        elif k == "new":
            self.new_ship()
        elif k == "load":
            self.ship_to_load_selected(self.craft_paths[c[1]])
        elif k == "inventory":
            self.release_part_to_icon()
        elif k == "allow":
            self.allowSrf = bool(c[1])
            self.allowNode = bool(c[2])
        elif k == "destroyHeld":
            self.delete_part(self.selectedPart)
        elif k == "rootSelect":
            self.on_new_root_select(self.part_by_id(c[1]))
        elif k == "offsetStart":
            self.offset_start()
        elif k == "offsetMove":
            self.offset_move(tuple(F(x) for x in c[1]), F(c[2]))
        elif k == "offsetEnd":
            self.offset_end()
        elif k == "rotateStart":
            self.rotate_start()
        elif k == "rotateMove":
            self.rotate_move(tuple(F(x) for x in c[1]), F(c[2]))
        elif k == "rotateEnd":
            self.rotate_end(tuple(F(x) for x in c[1]), F(c[2]))
        else:
            raise ValueError(k)


# ControlTypes.cs values the editor reads
CT = {"UI": 0xF000000000000000, "EDITOR_ICON_HOVER": 0x20000000, "EDITOR_ICON_PICK": 0x40000000, "EDITOR_TAB_SWITCH": 0x80000000,
      "EDITOR_LOAD": 0x200000000, "EDITOR_EXIT": 0x400000000, "EDITOR_PAD_PICK_PLACE": 0x2000000000, "EDITOR_GIZMO_TOOLS": 0x8000000000,
      "EDITOR_ROOT_REFLOW": 0x10000000000, "EDITOR_SYM_SNAP_UI": 0x20000000000, "EDITOR_UNDO_REDO": 0x100000000000,
      "GUI": 0x8000000000000}
