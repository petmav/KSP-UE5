#!/usr/bin/env python3
"""WP-40b: scripted editor sessions -> tests/data/wp40b_reference.json (test data generator for tests/test_editor_fsm.cpp).

Runs the independent simulator of session_ref_lib.py (written from the decompiled C#, no kspcore code) over scripted sessions and writes, per
frame: the inputs (mouse, buttons, keys, host commands), the ordered trace (FSM events and state changes, GameEvents of the editor, audio clips,
highlight calls, tweak hooks), a SHA-1 of the canonical snapshot of every part (position, rotation, attPos / attPos0 / attRotation / attRotation0
bits, tree, symmetry sets, attach node links, surface node link, frozen flag, ship membership), and the ordered log of every scene-seam call with its
analytic-fixture answer. The C++ test replays the inputs, answers the seam calls from the log (asserting the same question) and compares the trace
and the snapshot hashes of every frame exactly.

Usage: python -I core/tools/wp40/session_reference.py > core/tests/data/wp40b_reference.json
"""
import hashlib
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import session_ref_lib as L  # noqa: E402

E = L.E
F, f, v3 = L.F, L.f, L.v3
hexf, ZERO = L.hexf, L.ZERO
vadd, vsub, vmul, vnormalized = L.vadd, L.vsub, L.vmul, L.vnormalized

ROOT = E.ROOT
DB = E.load_part_db()
CRAFTS = {"flea": os.path.join(ROOT, "harness", "inputs", "Ships", "VAB", "Jumping Flea.craft"),
          "ion": os.path.join(ROOT, "harness", "inputs", "Ships", "VAB", "Ion-Powered Space Probe.craft")}
CAM = L.Sim.CAMERA
L.Sim.craft_paths = CRAFTS


def fnv1a64(text):
    h = 0xcbf29ce484222325
    for b in text.encode("utf-8"):
        h ^= b
        h = (h * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return "%016x" % h

STATES = ["st_podSelect", "st_idle", "st_place", "st_offset_select", "st_offset_tweak", "st_rotate_select", "st_rotate_tweak",
          "st_root_unselected", "st_root_select"]
EVENTS = ("on_podSelect on_podDeleted on_partCreated on_partPicked on_partCopied on_partReveal on_partDropped on_partAttached on_partDeleted "
          "on_partLost on_partOverInventoryPAW on_goToModeOffset on_offsetSelect on_offsetDeselect on_offsetReset on_goToModeRotate "
          "on_rotateSelect on_rotateDeselect on_rotateReset on_goToModeRoot on_rootPickSet on_rootDeselect on_rootSelect on_rootSelectFail "
          "on_goToModePlace on_undoRedo on_newShip on_shipLoaded").split()


def encode_cmd(c):
    """commands as recorded in the JSON: float arguments as float32 bit patterns"""
    out = [c[0]]
    for a in c[1:]:
        if isinstance(a, (list, tuple)):
            out.append([hexf(F(x)) for x in a])
        elif isinstance(a, float):
            out.append(hexf(F(a)))
        else:
            out.append(a)
    return out


class Sc:
    """a scripted session: every step() runs one engine frame in the simulator and records it"""

    def __init__(self, name, facility="VAB", note=""):
        self.name = name
        self.note = note
        self.sim = L.Sim(DB, facility)
        self.facility = facility
        self.frames = []
        self.tok = 0
        self.mouse = (F(640.0), F(360.0), F(0.0))
        self.nvals = 0
        L.USED.clear()
        self.sim.start()
        snap, nv = self.sim.snapshot_string()
        self.start = {"trace": list(self.sim.trace), "hash": fnv1a64(snap)}
        self.nvals += nv
        self.sim.trace.clear()
        self.seam_start = 0
        self.snaps = [snap]

    # ---- aiming: register a camera ray for a fresh screen position ----
    def aim(self, target):
        self.tok += 1
        sp = (F(640 + 3 * self.tok), F(360.0), F(0.0))
        key = "%s,%s,%s" % (hexf(sp[0]), hexf(sp[1]), hexf(sp[2]))
        self.sim.rays[key] = (CAM, vnormalized(vsub(target, CAM)))
        self.mouse = sp
        return sp

    def aim_for_position(self, target, iters=8):
        """aim so that dragOverPlane puts the dragged part at `target` (fixed point of the aim point)"""
        aim_pt = target
        for _ in range(iters):
            d = vnormalized(vsub(aim_pt, CAM))
            p = self.sim.drag_position(d)
            err = vsub(target, p)
            if float(L.mag(err)) < 1e-4:
                break
            aim_pt = vadd(aim_pt, err)
        return self.aim(aim_pt)

    def aim_part(self, part, off=ZERO):
        return self.aim(vadd(part.pos, off))

    # ---- stepping ----
    def step(self, cmds=(), lmb=None, rmb=None, kd=(), ku=(), kh=(), mouse=None, dt=5.0, n=1, ui=False):
        for _ in range(n):
            self._frame(cmds, lmb, rmb, kd, ku, kh, mouse if mouse is not None else self.mouse, dt, ui)
            cmds = ()
            lmb = rmb = None if lmb != "held" else lmb
            kd = ku = ()

    def _frame(self, cmds, lmb, rmb, kd, ku, kh, mouse, dt, ui=False):
        mb_down, mb_up, mb_held = [False] * 3, [False] * 3, [False] * 3
        for idx, st in ((0, lmb), (1, rmb)):
            if st == "down":
                mb_down[idx] = mb_held[idx] = True
            elif st == "up":
                mb_up[idx] = True
            elif st == "held":
                mb_held[idx] = True
        sim = self.sim
        sim.trace.clear()
        keys = (sorted(kd), sorted(ku), sorted(set(kh) | set(kd)))
        sim.frame(dt, mouse, (mb_down, mb_up, mb_held), keys, [list(c) for c in cmds], ui)
        snap, nv = sim.snapshot_string()
        self.nvals += nv
        self.frames.append({
            "dt": hexf(F(dt)), "mouse": [hexf(c) for c in mouse], "md": mb_down, "mu": mb_up, "mh": mb_held,
            "kd": keys[0], "ku": keys[1], "kh": keys[2], "ui": ui, "cmds": [encode_cmd(c) for c in cmds],
            "trace": list(sim.trace), "hash": fnv1a64(snap)})
        self.snaps.append(snap)

    # ---- gestures ----
    def click(self, mouse=None, **kw):
        m = mouse if mouse is not None else self.mouse
        self.step(lmb="down", mouse=m, **kw)
        self.step(lmb="up", mouse=m)

    def settle(self, n=2, **kw):
        self.step(n=n, **kw)

    def state(self):
        return self.sim.fsm.cur_name

    def part(self, pid):
        return self.sim.part_by_id(pid)

    def out(self):
        snap, nv = self.sim.snapshot_string()
        trace_events = set()
        for fr in [{"trace": self.start["trace"]}] + self.frames:
            for t in fr["trace"]:
                trace_events.add(t)
        return {"name": self.name, "note": self.note, "facility": self.facility, "start": self.start, "frames": self.frames,
                "seam": self.sim.seam, "final": snap, "valuesCompared": self.nvals, "unverified": sorted(L.USED)}, trace_events


SESSIONS = []


def session(fn):
    SESSIONS.append(fn)
    return fn


def key(s, k, held=(), n=1, **kw):
    for _ in range(n):
        s.step(kd=[k], kh=list(held), **kw)


def pod(s):
    s.step(cmds=[("spawn", "mk1pod.v2")])
    s.settle(2)
    return s.sim.rootPart


def stack_part(s, name, node_in="top", node_out="bottom", keys=(), ship_part=None):
    """spawn `name`, drag it so that its `node_in` meets the `node_out` node of `ship_part` (default: the last part of the ship), drop it"""
    sim = s.sim
    tgt = ship_part if ship_part is not None else sim.ship[-1]
    s.step(cmds=[("spawn", name), ("allow", 0, 1)], mouse=s.aim_for_position(v3(0.0, 0.0, 0.0)))
    me = sim.selectedPart
    n_in = [n for n in me.nodes if n.id == node_in][0]
    n_out = [n for n in tgt.nodes if n.id == node_out][0]
    target = vsub(tgt.tp(n_out.position), n_in.position)
    m = s.aim_for_position(target)
    s.settle(3, mouse=m, kd=list(keys))
    s.click(mouse=m)
    s.step(cmds=[("allow", 1, 1)])
    s.settle(1)
    return me


def hold(s, name, target=None):
    m = s.aim_for_position(target if target is not None else v3(6.0, 9.0, 3.0))
    s.step(cmds=[("spawn", name)], mouse=m)
    return s.sim.selectedPart, m


def attach_srf(s, name, tgt, off=ZERO, keys=(), settle=3, kh=()):
    """spawn `name` and drop it on the surface of `tgt` (the cursor ray goes through the part, the surface seam answers the hit)"""
    m = s.aim(vadd(tgt.pos, off))
    s.step(cmds=[("spawn", name)], mouse=m)
    me = s.sim.selectedPart
    s.settle(settle, mouse=m, kd=list(keys), kh=list(kh))
    s.click(mouse=m)
    s.settle(2)
    return me


def click_on(s, part, off=ZERO, kh=(), pre=2, post=1, ui=False):
    m = s.aim(vadd(part.pos, off))
    s.settle(pre, mouse=m, kh=list(kh))
    s.step(lmb="down", mouse=m, kh=list(kh), ui=ui)
    s.step(lmb="up", mouse=m, kh=list(kh), ui=ui)
    s.settle(post)
    return m


def click_empty(s, pre=2, post=1, kh=()):
    m = s.aim(v3(9.0, 12.0, 9.0))
    s.settle(pre, mouse=m, kh=list(kh))
    s.step(lmb="down", mouse=m, kh=list(kh))
    s.step(lmb="up", mouse=m, kh=list(kh))
    s.settle(post)
    return m


def three_part_ship(s):
    pod(s)
    stack_part(s, "fuelTankSmallFlat")
    stack_part(s, "liquidEngine2")
    return s.sim.ship


def tank_with_radials(s, n_keys=2, name="radialDecoupler", keys_kh=()):
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    for _ in range(n_keys):
        key(s, "X")
    me = attach_srf(s, name, tank, off=v3(0.2, 0.0, -0.3))
    return tank, me


@session
def s01_pod_and_tank():
    s = Sc("s01_pod_and_tank", note="spawn the pod (podSelect to idle), stack a tank under it with a node attach")
    pod(s)
    stack_part(s, "fuelTankSmallFlat")
    return s


@session
def s02_stack_chain_and_undo():
    s = Sc("s02_stack_chain_and_undo", note="pod, tank, engine; undo three times (Ctrl+Z), redo twice (Ctrl+Y); undo limit clamps")
    three_part_ship(s)
    for _ in range(3):
        key(s, "Z", held=["LeftControl"])
    key(s, "Z", held=["LeftControl"])
    for _ in range(2):
        key(s, "Y", held=["LeftControl"])
    key(s, "Y", held=["LeftControl"])
    key(s, "Y", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s03_radial_symmetry_three():
    s = Sc("s03_radial_symmetry_three", note="three-way radial symmetry (X twice), surface attach on a tank, drop and attach")
    tank_with_radials(s, 2)
    return s


@session
def s04_radial_symmetry_eight_world():
    s = Sc("s04_radial_symmetry_eight_world", note="eight-way radial symmetry in world coordinates (F), previous mode with Shift+X")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    key(s, "X", held=["LeftShift"])
    key(s, "F")
    key(s, "X", held=["LeftShift"])
    me = attach_srf(s, "radialDecoupler", tank, off=v3(-0.3, 0.2, 0.2))
    return s


@session
def s05_mirror_symmetry():
    s = Sc("s05_mirror_symmetry", note="mirror method (R released), one mirror counterpart on a tank")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    s.step(ku=["R"])
    key(s, "X")
    attach_srf(s, "radialDecoupler", tank, off=v3(0.1, 0.1, -0.2))
    return s


@session
def s06_stack_symmetry_forced():
    s = Sc("s06_stack_symmetry_forced", note="stack part under a part with stackSymmetry 1: the symmetry mode is forced and restored")
    pod(s)
    bi = stack_part(s, "stackBiCoupler.v2")
    s.step(kd=["X"])
    stack_part(s, "fuelTankSmallFlat", node_out="bottom1")
    return s


@session
def s07_angle_snap_surface():
    s = Sc("s07_angle_snap_surface", note="angle snap on (C), surface attach goes through FindPartSurface; fine tweak held")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    key(s, "C")
    attach_srf(s, "batteryPack", tank, off=v3(0.2, 0.0, 0.1))
    attach_srf(s, "batteryPack", tank, off=v3(-0.2, 0.1, 0.0), kh=["LeftShift"])
    key(s, "C")
    return s


@session
def s08_rotate_keys():
    s = Sc("s08_rotate_keys", note="keyboard rotation of the held part (yaw, roll, pitch, fine), reset with Space")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(2, mouse=m)
    for k in ("A", "D", "Q", "E", "S", "W"):
        key(s, k, mouse=m)
    key(s, "A", held=["LeftShift"], mouse=m)
    key(s, "Space", mouse=m)
    s.settle(1, mouse=m)
    return s


@session
def s09_drop_free_and_pick():
    s = Sc("s09_drop_free_and_pick", note="drop a part in free space (on_partDropped, frozen), pick it again (on_partPicked), reattach")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(3)
    click_on(s, part)
    s.settle(2)
    tgt = s.sim.rootPart
    m2 = s.aim_for_position(vsub(tgt.tp([n for n in tgt.nodes if n.id == "bottom"][0].position), [n for n in part.nodes if n.id == "top"][0].position))
    s.settle(3, mouse=m2)
    s.click(mouse=m2)
    s.settle(2)
    return s


@session
def s10_pick_attached_part_and_move():
    s = Sc("s10_pick_attached_part_and_move", note="pick an attached tank (detach + backup), drag it away, drop it on the pod top node")
    ships = three_part_ship(s)
    tank = ships[1]
    click_on(s, tank)
    s.settle(2)
    pod_ = s.sim.rootPart
    top = [n for n in pod_.nodes if n.id == "top"][0]
    mine = [n for n in tank.nodes if n.id == "bottom"][0]
    m = s.aim_for_position(vsub(pod_.tp(top.position), mine.position))
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(2)
    return s


@session
def s11_copy_part():
    s = Sc("s11_copy_part", note="Alt+click copies a part (on_partCopied), the copy is held and attached on the tank surface")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    deco = attach_srf(s, "radialDecoupler", tank, off=v3(0.3, 0.0, -0.2))
    click_on(s, deco, kh=["LeftAlt"])
    m = s.aim(vadd(tank.pos, v3(-0.3, 0.1, 0.1)))
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(2)
    return s


@session
def s12_reveal_part():
    s = Sc("s12_reveal_part", note="Ctrl+click reveals the part in the part list (on_partReveal)")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    click_on(s, tank, kh=["LeftControl"])
    s.settle(2)
    return s


@session
def s13_delete_key_held_part():
    s = Sc("s13_delete_key_held_part", note="Delete while holding a part (DestroySelectedPart, on_partDeleted)")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(2, mouse=m)
    key(s, "Delete", mouse=m)
    s.settle(2)
    return s


@session
def s14_delete_the_pod():
    s = Sc("s14_delete_the_pod", note="pick the only part (root) and delete it: on_podDeleted, st_podSelect; undo restores it")
    p = pod(s)
    click_on(s, p)
    s.settle(2)
    key(s, "Delete")
    s.settle(2)
    key(s, "Z", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s15_part_over_inventory():
    s = Sc("s15_part_over_inventory", note="the held part is released to an inventory grid (on_partOverInventoryPAW)")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(2, mouse=m)
    s.step(cmds=[("inventory",)], mouse=m)
    s.settle(2)
    return s


@session
def s16_part_lost():
    s = Sc("s16_part_lost", note="the held part is destroyed behind the editor back (on_partLost)")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(2, mouse=m)
    s.step(cmds=[("destroyHeld",)], mouse=m)
    s.settle(2)
    return s


@session
def s17_new_ship():
    s = Sc("s17_new_ship", note="New ship with unsaved changes (dialog seam answers Discard): on_newShip, st_podSelect; then a new pod")
    three_part_ship(s)
    s.step(cmds=[("new",)])
    s.settle(2)
    pod(s)
    stack_part(s, "fuelTankSmallFlat")
    return s


@session
def s18_load_ship_and_edit():
    s = Sc("s18_load_ship_and_edit", note="load the stock craft Jumping Flea (on_shipLoaded), pick and delete a part, undo and redo")
    s.step(cmds=[("load", "flea")])
    s.settle(3)
    flea = s.sim.ship
    click_on(s, flea[1])
    s.settle(2)
    key(s, "Delete")
    s.settle(2)
    key(s, "Z", held=["LeftControl"])
    key(s, "Z", held=["LeftControl"])
    key(s, "Y", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s19_load_second_craft_then_new_clean():
    s = Sc("s19_load_second_craft_then_new_clean", note="load the ion probe, load again (replaces), New ship without unsaved changes (no dialog)")
    s.step(cmds=[("load", "ion")])
    s.settle(3)
    s.step(cmds=[("load", "flea")])
    s.settle(3)
    s.step(cmds=[("new",)])
    s.settle(2)
    return s


@session
def s20_undo_with_held_part():
    s = Sc("s20_undo_with_held_part", note="undo while a part is held: skipPartAttach for one frame (coroutine)")
    ships = three_part_ship(s)
    click_on(s, ships[2])
    s.settle(2)
    key(s, "Z", held=["LeftControl"])
    s.settle(3)
    return s


@session
def s21_undo_limit():
    s = Sc("s21_undo_limit", note="more backups than the undo limit: the oldest are shifted out; undo to the oldest and back")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    for i in range(18):
        click_on(s, tank)
        s.settle(2)
        pod_ = s.sim.rootPart
        top = [n for n in pod_.nodes if n.id == "bottom"][0]
        mine = [n for n in tank.nodes if n.id == "top"][0]
        m = s.aim_for_position(vsub(pod_.tp(top.position), mine.position))
        s.settle(3, mouse=m)
        s.click(mouse=m)
        s.settle(2)
        tank = s.sim.ship[-1]
    for _ in range(3):
        key(s, "Z", held=["LeftControl"])
    key(s, "Y", held=["LeftControl"])
    return s


@session
def s22_symmetry_pick_and_delete():
    s = Sc("s22_symmetry_pick_and_delete", note="pick one of three symmetric parts (counterparts deleted), then hold and Delete")
    tank, me = tank_with_radials(s, 2)
    click_on(s, me)
    s.settle(2)
    key(s, "Delete")
    s.settle(2)
    return s


@session
def s23_offset_mode_gizmo():
    s = Sc("s23_offset_mode_gizmo", note="offset mode: select a part, drag the offset gizmo, deselect, back to place mode")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    tank = ships[1]
    click_on(s, tank)
    s.settle(1)
    s.step(cmds=[("offsetStart",), ("offsetMove", [0.0, 1.0, 0.0], 0.3), ("offsetMove", [1.0, 0.0, 0.0], -0.2), ("offsetEnd",)])
    s.settle(2)
    key(s, "F")
    s.step(kd=["F"], ku=["F"])
    s.step(cmds=[("offsetStart",), ("offsetMove", [0.0, 0.0, 1.0], 0.1), ("offsetEnd",)])
    click_empty(s)
    s.settle(2)
    s.step(cmds=[("mode", 0)])
    s.settle(2)
    return s


@session
def s24_offset_reset_and_undo():
    s = Sc("s24_offset_reset_and_undo", note="offset tweak: Space resets (on_offsetReset), Ctrl+Z leaves the tweak (on_undoRedo)")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    click_on(s, ships[1])
    s.settle(1)
    s.step(cmds=[("offsetStart",), ("offsetMove", [0.0, 1.0, 0.0], 0.25), ("offsetEnd",)])
    key(s, "Space")
    s.settle(2)
    key(s, "Z", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s25_rotate_mode_gizmo():
    s = Sc("s25_rotate_mode_gizmo", note="rotate mode: select a part, drag the rotate gizmo (self and world), reset with Space, deselect")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 2)])
    s.settle(2)
    click_on(s, ships[1])
    s.settle(1)
    s.step(cmds=[("rotateStart",), ("rotateMove", [0.0, 1.0, 0.0], 30.0), ("rotateMove", [0.0, 1.0, 0.0], 45.0), ("rotateEnd", [0.0, 1.0, 0.0], 45.0)])
    s.step(kd=["F"], ku=["F"])
    s.step(cmds=[("rotateStart",), ("rotateMove", [1.0, 0.0, 0.0], 15.0), ("rotateEnd", [1.0, 0.0, 0.0], 15.0)])
    key(s, "Space")
    s.settle(2)
    click_empty(s)
    s.settle(2)
    return s


@session
def s26_rotate_symmetric_parts():
    s = Sc("s26_rotate_symmetric_parts", note="rotate tweak of a part with radial counterparts: the symmetry follows (UpdateSymmetry)")
    tank, me = tank_with_radials(s, 2)
    s.step(cmds=[("mode", 2)])
    s.settle(2)
    click_on(s, me)
    s.settle(1)
    s.step(cmds=[("rotateStart",), ("rotateMove", [0.0, 0.0, 1.0], 20.0), ("rotateEnd", [0.0, 0.0, 1.0], 20.0)])
    s.step(cmds=[("mode", 1)])
    s.settle(1)
    s.step(cmds=[("offsetStart",), ("offsetMove", [0.0, 1.0, 0.0], 0.1), ("offsetEnd",)])
    s.settle(2)
    return s


@session
def s27_tweak_mode_switch_and_delete():
    s = Sc("s27_tweak_mode_switch_and_delete", note="offset to rotate tweak with a selected part, Delete in the tweak state (on_partDeleted)")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    click_on(s, ships[2])
    s.settle(1)
    s.step(cmds=[("mode", 2)])
    s.settle(2)
    key(s, "Delete")
    s.settle(2)
    return s


@session
def s28_rotate_undo():
    s = Sc("s28_rotate_undo", note="rotate tweak then Ctrl+Z: st_rotate_select; Ctrl+Y; leave with mode place")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 2)])
    s.settle(2)
    click_on(s, ships[1])
    s.settle(1)
    s.step(cmds=[("rotateStart",), ("rotateMove", [0.0, 1.0, 0.0], 25.0), ("rotateEnd", [0.0, 1.0, 0.0], 25.0)])
    key(s, "Z", held=["LeftControl"])
    s.settle(2)
    key(s, "Y", held=["LeftControl"])
    s.step(cmds=[("mode", 0)])
    s.settle(2)
    return s


@session
def s29_rotate_select_pick_held_part():
    s = Sc("s29_rotate_select_pick_held_part", note="rotate select, click empty space (no part), then mode offset from rotate select, offset select pick, mode place")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 2)])
    s.settle(2)
    click_empty(s)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    click_empty(s)
    click_on(s, ships[1])
    s.settle(1)
    s.step(cmds=[("mode", 0)])
    s.settle(2)
    return s


@session
def s30_root_mode_reroot():
    s = Sc("s30_root_mode_reroot", note="root mode: pick a part with children (on_rootPickSet), choose a new root (on_rootSelect), place it")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 3)])
    s.settle(2)
    click_on(s, ships[1], kh=[])
    s.settle(2)
    cands = list(s.sim.rootCandidates)
    if cands:
        s.step(cmds=[("rootSelect", cands[0].id)])
        s.settle(3)
    s.settle(2)
    return s


@session
def s31_root_mode_deselect_and_fail():
    s = Sc("s31_root_mode_deselect_and_fail", note="root mode: click a part without children (refused), pick one with children, click empty (on_rootDeselect)")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 3)])
    s.settle(2)
    click_on(s, ships[2])
    s.settle(2)
    click_on(s, ships[0])
    s.settle(2)
    click_empty(s)
    s.settle(2)
    s.step(cmds=[("mode", 0)])
    s.settle(2)
    return s


@session
def s32_root_mode_from_held_part():
    s = Sc("s32_root_mode_from_held_part", note="root mode while holding a part: on_goToModeRoot to st_root_select with the held hierarchy, then back to place")
    pod(s)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(3, mouse=m)
    s.step(cmds=[("mode", 3)], mouse=m)
    s.settle(3, mouse=m)
    s.step(cmds=[("mode", 0)], mouse=m)
    s.settle(2, mouse=m)
    return s


@session
def s33_mode_keys():
    s = Sc("s33_mode_keys", note="construction modes through the 1 2 3 4 keys with a selected part in between")
    ships = three_part_ship(s)
    key(s, "Alpha2")
    s.settle(1)
    click_on(s, ships[1])
    key(s, "Alpha3")
    s.settle(1)
    key(s, "Alpha4")
    s.settle(1)
    key(s, "Alpha1")
    s.settle(2)
    return s


@session
def s34_symmetry_keys():
    s = Sc("s34_symmetry_keys", note="symmetry mode, method and coordinate keys in idle and while holding a part")
    pod(s)
    for _ in range(3):
        key(s, "X")
    s.step(ku=["R"])
    key(s, "X")
    key(s, "F")
    s.step(ku=["R"])
    key(s, "F")
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(2, mouse=m)
    key(s, "X", mouse=m)
    key(s, "F", mouse=m)
    s.step(ku=["R"], mouse=m)
    key(s, "C", mouse=m)
    key(s, "C", mouse=m)
    return s


@session
def s35_sph_mirror_wing():
    s = Sc("s35_sph_mirror_wing", facility="SPH", note="SPH facility, mirror symmetry of a surface part on a fuselage")
    pod(s)
    stack_part(s, "fuelTankSmallFlat")
    tank = s.sim.ship[-1]
    s.step(ku=["R"])
    key(s, "X")
    attach_srf(s, "batteryPack", tank, off=v3(0.15, 0.0, -0.2))
    return s


@session
def s36_two_level_symmetry():
    s = Sc("s36_two_level_symmetry", note="a part on the surface of a symmetric part gets the symmetry of its parent")
    tank, deco = tank_with_radials(s, 2)
    key(s, "X")
    attach_srf(s, "batteryPack", deco, off=v3(0.1, 0.1, 0.0))
    return s


@session
def s37_pointer_over_ui():
    s = Sc("s37_pointer_over_ui", note="clicks while the pointer is over the UI do nothing; the same clicks over the scene do")
    ships = three_part_ship(s)
    click_on(s, ships[1], ui=True)
    s.settle(2)
    part, m = hold(s, "fuelTankSmallFlat")
    s.settle(3, mouse=m)
    s.step(lmb="down", mouse=m, ui=True)
    s.step(lmb="up", mouse=m, ui=True)
    s.settle(2, mouse=m)
    s.click(mouse=m)
    s.settle(2)
    return s


@session
def s38_right_drag_and_left_drag():
    s = Sc("s38_right_drag_and_left_drag", note="a right-button drag clears the surface cursor offset; a left click that was a drag does not pick")
    ships = three_part_ship(s)
    m1 = s.aim(vadd(ships[1].pos, ZERO))
    s.settle(2, mouse=m1)
    s.step(lmb="down", mouse=m1)
    far = (F(900.0), F(500.0), F(0.0))
    s.step(lmb="held", mouse=far)
    s.step(lmb="up", mouse=far)
    s.settle(2)
    s.step(rmb="down", mouse=m1)
    s.step(rmb="held", mouse=far)
    s.step(rmb="up", mouse=far)
    s.settle(2)
    return s


@session
def s39_shift_pick_subtree():
    s = Sc("s39_shift_pick_subtree", note="Shift+click picks the root of a frozen subtree: a whole branch is moved")
    ships = three_part_ship(s)
    part, m = hold(s, "fuelTankSmallFlat", target=v3(7.0, 9.0, 2.0))
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(3)
    click_on(s, part, kh=["LeftShift"])
    s.settle(2)
    click_empty(s)
    return s


@session
def s40_new_ship_cancel_paths():
    s = Sc("s40_new_ship_cancel_paths", note="load then edit then New ship: the dialog path with discard; NewShip from st_podSelect")
    s.step(cmds=[("load", "flea")])
    s.settle(3)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    click_on(s, s.sim.ship[1])
    s.settle(1)
    s.step(cmds=[("new",)])
    s.settle(2)
    s.step(cmds=[("new",)])
    s.settle(2)
    return s


@session
def s41_two_parts_same_node():
    s = Sc("s41_two_parts_same_node", note="a second part dropped on an already used node is refused (not possible), dropped free instead")
    pod(s)
    stack_part(s, "fuelTankSmallFlat")
    stack_part(s, "fuelTankSmallFlat", ship_part=s.sim.rootPart, node_out="bottom")
    return s


@session
def s42_pod_selection_then_other_pod():
    s = Sc("s42_pod_selection_then_other_pod", note="delete the pod, spawn another root part, build on it")
    p = pod(s)
    click_on(s, p)
    s.settle(2)
    key(s, "Delete")
    s.settle(2)
    s.step(cmds=[("spawn", "fuelTankSmallFlat")])
    s.settle(2)
    stack_part(s, "liquidEngine2")
    return s


@session
def s43_srf_part_picked_and_dropped_on_other_tank():
    s = Sc("s43_srf_part_picked_and_dropped_on_other_tank", note="a surface part moved from one tank to another (pick, node clearing, attach)")
    pod(s)
    t1 = stack_part(s, "fuelTankSmallFlat")
    t2 = stack_part(s, "fuelTankSmallFlat")
    b = attach_srf(s, "batteryPack", t1, off=v3(0.2, 0.0, 0.0))
    click_on(s, b)
    s.settle(1)
    m = s.aim(vadd(t2.pos, v3(0.0, 0.0, 0.2)))
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(2)
    return s


@session
def s44_mirror_then_radial_switch():
    s = Sc("s44_mirror_then_radial_switch", note="switch the symmetry method while a symmetric part is held (temporary method, restore)")
    pod(s)
    tank = stack_part(s, "fuelTankSmallFlat")
    key(s, "X")
    key(s, "X")
    me = attach_srf(s, "radialDecoupler", tank, off=v3(0.2, 0.0, -0.2), settle=2)
    click_on(s, me)
    s.step(ku=["R"])
    s.settle(2)
    key(s, "X")
    m = s.aim(vadd(tank.pos, v3(-0.2, 0.0, 0.3)))
    s.settle(3, mouse=m)
    s.click(mouse=m)
    s.settle(2)
    return s


@session
def s45_idle_keys_and_noop_clicks():
    s = Sc("s45_idle_keys_and_noop_clicks", note="idle: Delete, undo without backups, clicks into the void, Ctrl+Y with nothing to redo")
    pod(s)
    key(s, "Delete")
    click_empty(s)
    key(s, "Y", held=["LeftControl"])
    key(s, "Z", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s46_podselect_undo_redo():
    s = Sc("s46_podselect_undo_redo", note="undo / redo while in st_podSelect after deleting every part")
    p = pod(s)
    stack_part(s, "fuelTankSmallFlat")
    click_on(s, s.sim.ship[1])
    s.settle(1)
    key(s, "Delete")
    click_on(s, s.sim.rootPart)
    s.settle(1)
    key(s, "Delete")
    s.settle(2)
    key(s, "Z", held=["LeftControl"])
    s.settle(2)
    key(s, "Y", held=["LeftControl"])
    s.settle(2)
    return s


@session
def s47_attached_refused_counterpart():
    s = Sc("s47_attached_refused_counterpart", note="stack symmetry with a part whose counterpart nodes collide: the drop is refused or accepted exactly as the C# says")
    pod(s)
    bi = stack_part(s, "stackBiCoupler.v2")
    s.step(kd=["X"])
    stack_part(s, "fuelTankSmallFlat", node_out="bottom1")
    stack_part(s, "fuelTankSmallFlat", ship_part=bi, node_out="bottom2")
    return s


@session
def s48_world_space_toggle_in_tweak():
    s = Sc("s48_world_space_toggle_in_tweak", note="coordinate space toggled (F released) in offset and rotate tweak, offset gizmo dragged in world space")
    ships = three_part_ship(s)
    s.step(cmds=[("mode", 1)])
    s.settle(2)
    click_on(s, ships[2])
    s.settle(1)
    s.step(ku=["F"])
    s.step(cmds=[("offsetStart",), ("offsetMove", [1.0, 0.0, 0.0], 0.2), ("offsetEnd",)])
    s.step(cmds=[("mode", 2)])
    s.settle(1)
    s.step(ku=["F"])
    s.step(cmds=[("rotateStart",), ("rotateMove", [0.0, 1.0, 0.0], 10.0), ("rotateEnd", [0.0, 1.0, 0.0], 10.0)])
    s.settle(2)
    return s


def main():
    dump = None
    if "--dump" in sys.argv:
        k = sys.argv.index("--dump")
        dump = (sys.argv[k + 1], int(sys.argv[k + 2]))
    out = []
    seen = set()
    for fn in SESSIONS:
        s = fn()
        if dump is not None and s.name == dump[0]:
            print(s.snaps[dump[1]])
            print("\n".join(s.frames[dump[1] - 1]["trace"]) if dump[1] > 0 else "\n".join(s.start["trace"]))
            return
        o, tr = s.out()
        out.append(o)
        seen |= tr
        st = []
        for t in sorted(tr):
            if t.startswith("st:"):
                st.append(t[3:])
        print("%-44s frames %3d seam %4d  %s" % (s.name, len(s.frames), len(s.sim.seam), s.state()), file=sys.stderr)
    states_seen = set()
    events_seen = set()
    for t in seen:
        if t.startswith("st:"):
            a, b = t[3:].split(">")
            states_seen.add(a)
            states_seen.add(b)
        if t.startswith("ev:"):
            events_seen.add(t[3:])
    cov = {"statesMissing": [x for x in STATES if x not in states_seen], "eventsMissing": [x for x in EVENTS if x not in events_seen]}
    json.dump({"camera": [hexf(c) for c in CAM], "sessions": out, "coverage": cov}, sys.stdout, separators=(",", ":"))
    sys.stdout.write("\n")
    print(json.dumps(cov), file=sys.stderr)


if __name__ == "__main__":
    main()
