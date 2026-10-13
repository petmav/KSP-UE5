"""WP-32 reference: scripted scenarios driven through a SECOND implementation of the KerbalFSM runtime (KerbalFSM.cs updateFSM / RunEvent / StartFSM) over the
state / event tables PARSED from the C# (eva_tables.py), with Python versions of the event conditions (eva_ref_conditions.PRED plus the few below) and the
side effects of the OnEnter / OnEvent bodies that change what the conditions read. The C++ test applies the same scripted world to the live KerbalEVA, calls
UpdateFSM / FixedUpdateFSM / LateUpdateFSM and compares (state, last event) after every step.

Scenarios are chosen with inert callbacks (identity rotation, tgtFwd = forward, fUp = up) so the scripted world is what the conditions see.
"""
import copy

import eva_tables
from eva_common import anim_table, cfg_float
from eva_math import ROTATIONS, axes, bits, clamp, dot, f32, sqr_magnitude, v_approx_zero
from eva_ref_conditions import PRED, land_start_target, feet_wet_target, feet_dry_target, recover_complete_target, start_run_target, end_run_target, \
    bound_land_target, wire
from eva_ref_motion import b3

T = anim_table()
MODES = {0: "manual", 1: "update", 2: "fixed", 4: "late"}


def base_world():
    zero = (0.0, 0.0, 0.0)
    w = dict(tgt=zero, dh=0.0, ldh=0.0, gee=1.0, cf=False, landed=True, splashed=False, packed=False, active=True, hft=0.0, hfs=0.0, t=0.0, frames=0,
             lbs=f32(0.5), jet=False, pm=False, restricted=False, keys={}, noparent=True, cl=False, clrot=0, inv=False, ltr=zero, chk=False, ccl=True,
             cdn=True, spd=0.0, ntrig=0, lad_active=True, lad_enabled=True, cpath=False, cnt=0.0, cend=1.0, clen=1.0, inconstr=False, weldfx=False,
             weldmesh=False, airlock=False, rbv=zero, vrb=zero, lcd=zero, lcn=zero, fup=(0.0, 1.0, 0.0), rot=0, ut=1000.0, lct=0.0, hss=0.0)
    return w


# ---- extra predicates (events whose conditions have engine-side parts that answer "nothing there" in the scenario worlds) --------------------------------
def can_recover(w):
    rt = cfg_float("recoverThreshold")
    rbsq = sqr_magnitude(w["rbv"])
    ok = rbsq < rt * rt or ((not (w["landed"] or w["splashed"])) and w["ut"] > w["lct"] + float(cfg_float("recoverTime")))
    if ok:
        return (not w["active_is_self"]) or (not v_approx_zero(w["tgt"]))
    return False


EXTRA = {
    "Recover Start": lambda w, sim: can_recover(w) and sim.in_state_time() > 0.20000000298023224,
    "Ladder Grab Start": lambda w, sim: w["ntrig"] > 0 and (w["active"] and not w["packed"]) and not w["restricted"] and bool(w["keys"].get("Use", {}).get("down", False)),
    "Boarding Part": lambda w, sim: w["airlock"] and (w["active"] and not w["packed"]) and not w["restricted"]
    and (sim.current == "Ladder (Idle)" or w["ntrig"] == 0) and bool(w["keys"].get("Board", {}).get("up", False)),
    "Clamber Grab Start": lambda w, sim: False,
    "Control Panel Search": lambda w, sim: False,
    "Semi-Deploy Parachute": lambda w, sim: False,
}


class Sim:
    def __init__(self, tbl, world, start_state):
        self.t = tbl
        self.w = world
        self.events = {e["name"]: e for e in tbl["events"]}
        self.states = {s["name"]: s for s in tbl["states"]}
        self.now = f32(100.0)
        self.frame = 1000
        self.current = start_state
        self.enter = float(self.now)
        self.enter_frame = self.frame
        self.last_event = ""
        self.last_state = None
        self.dur = {}      # runtime TimerDuration overrides
        self.goto = {}     # runtime GoToStateOnEvent overrides
        self.log = []
        self.w["active_is_self"] = True

    def in_state_time(self):
        return float(self.now) - self.enter

    # -- durations
    def duration(self, name):
        if name in self.dur:
            return self.dur[name]
        e = self.events[name]
        d = e["duration"]
        if d == "Animations.flagPlant.State.length":
            return float(T["flagPlant"]["length"])
        if d == "Animations.weld.State.length":
            return float(T["weld"]["length"])
        return float(d)

    # -- conditions
    def condition(self, name):
        e = self.events[name]
        if e["timed"]:
            return float(self.now) - self.enter > self.duration(name)
        if name in EXTRA:
            return EXTRA[name](self.w, self)
        if name in PRED:
            w = dict(self.w)
            w["t"] = f32(self.in_state_time())
            w["frames"] = self.frame - self.enter_frame
            # PRED reads tstate(w) = (double)Time.time - TimeAtStateEnter: pass the exact double through "t" (a float32 in the reference worlds): use enter = 0 trick
            return self._pred(name, w)
        if e["updateMode"] == 0:
            return False
        raise KeyError("no predicate for event " + name)

    def _pred(self, name, w):
        # PRED's time inputs are the exact doubles of this sim (time - enter): pass them through float keys that tstate() reads
        w["t"] = self.in_state_time()
        return bool(PRED[name](w))

    # -- OnEvent effects
    def on_event(self, name):
        w = self.w
        e = self.events[name]
        if name == "Landing":
            ww = dict(up=axes(ROTATIONS[w["rot"]])[0], fup=w["fup"], lcd=w["lcd"], lcn=w["lcn"], vrb=w["vrb"], ut=w["ut"], lct=w["lct"])
            self.goto[name] = land_start_target(ww)
        elif name == "Feet Wet":
            self.goto[name] = feet_wet_target(dict(up=axes(ROTATIONS[w["rot"]])[0], fup=w["fup"], rbv=w["rbv"]))
        elif name == "Feet Dry":
            self.goto[name] = feet_dry_target(dict(up=axes(ROTATIONS[w["rot"]])[0], fup=w["fup"]))
        elif name == "Recover End":
            self.goto[name] = recover_complete_target(dict(landed=w["landed"], splashed=w["splashed"]))
        elif name == "Start Run":
            self.goto[name] = start_run_target(dict(gee=w["gee"], cf=w["cf"], prev=self.goto.get(name)))
        elif name == "End Run":
            self.goto[name] = end_run_target(dict(cf=w["cf"]))
        elif name == "Low G bound land":
            self.goto[name] = bound_land_target(dict(cf=w["cf"]))
        elif name in ("Ladder Let Go", "Ladder End"):
            w["cl"] = False
            w["ntrig"] = 0
        elif name == "Pack Toggle":
            self.goto[name] = self.current
            w["jet"] = not w["jet"]

    # -- OnEnter / OnLeave effects that touch condition inputs
    def on_enter(self, st, prev):
        w = self.w
        if st == "Jumping":
            self.dur["Jump Launch"] = 0.20000000298023224 if w["hss"] < 0.20000000298023224 else float(1.0)
            # (Jump Launch: JumpStillStart 0.2 / JumpFwdStart.end = 1)
        elif st == "Recover":
            fwd = axes(ROTATIONS[w["rot"]])[1]
            up = w["fup"]
            if dot(fwd, up) > 0.0:
                if w["splashed"] and not w["landed"]:
                    nm = "swimUpFaceUp"
                elif w["landed"]:
                    nm = "standUpFaceUp"
                else:
                    nm = "suspendedIdle"
            elif w["splashed"] and not w["landed"]:
                nm = "swimUpFaceDown"
            elif w["landed"]:
                nm = "standUpFaceDown"
            else:
                nm = "suspendedIdle"
            self.dur["Recover End"] = float(f32(T[nm]["length"] - f32(0.6)))
        elif st == "Ladder (Acquire)":
            w["cl"] = True
            w["inv"] = dot(axes(ROTATIONS[w["clrot"]])[0], axes(ROTATIONS[w["rot"]])[0]) < 0.0
        elif st in ("Ragdoll", "Grappled"):
            w["cl"] = False
            w["ntrig"] = 0
        elif st == "Low G Bound (Grounded - Arcade)":
            w["lbs"] = clamp(w["lbs"], cfg_float("boundFrequency"), 100.0)   # bound_gr_acd_OnEnter (:2207)
        elif st == "Entering construction mode":
            w["weldfx"] = True                                                # enteringConstruction_OnEnter creates the weld tool; EnableMesh
            w["weldmesh"] = True
        elif st == "Exiting construction mode":
            w["weldmesh"] = False                                             # DisableMesh

    def on_leave(self, st, nxt):
        if st == "Low G Bound (floating)":
            self.w["lbs"] = f32(self.in_state_time())

    # -- the runtime
    def run_event(self, name):
        if name not in self.states[self.current]["events"]:
            self.log.append("Event %s not assigned to state %s" % (name, self.current))
            return
        self.on_event(name)
        e = self.events[name]
        target = self.goto.get(name, e["goto"])
        self.last_event = name
        prev = self.current
        self.on_leave(prev, target)
        self.last_state = prev
        self.current = target
        self.enter = float(self.now)
        self.enter_frame = self.frame
        self.on_enter(target, prev)

    def callbacks(self, mode):
        # state callbacks that run RunEvent themselves (enteringExitingConstruction_OnFixedUpdate)
        st = self.current
        names = {2: "onFixedUpdate", 1: "onUpdate", 4: "onLateUpdate"}[mode]
        for cb in self.states[st][names]:
            if cb == "enteringExitingConstruction_OnFixedUpdate":
                if self.w["landed"]:
                    if (st == "Entering construction mode" and self.in_state_time() >= float(T["weldGunDraw"]["length"])) or \
                            (st == "Exiting construction mode" and self.in_state_time() >= float(T["weldGunPutAway"]["length"])):
                        self.run_event("Construction Mode Toggle finished on ground")
                        return
                else:
                    if (st == "Entering construction mode" and self.in_state_time() >= float(T["weldGunDraw"]["length"])) or \
                            (st == "Exiting construction mode" and self.in_state_time() >= float(T["weldGunPutAway"]["length"])):
                        self.run_event("Construction Mode Toggle finished on space")
                        return

    def update_fsm(self, mode):
        self.callbacks(mode)
        i = 0
        while i < len(self.states[self.current]["events"]):
            name = self.states[self.current]["events"][i]
            e = self.events[name]
            if e["updateMode"] == mode and self.condition(name):
                self.run_event(name)
            i += 1


def scenario(name, start, steps, tbl):
    """steps: list of dicts: mode (1 update / 2 fixed / 4 late) | event (RunEvent), dt, set (world changes)"""
    w = base_world()
    sim = Sim(tbl, w, start)
    trace = []
    for st in steps:
        if "set" in st:
            for k, v in st["set"].items():
                if k == "keys":
                    w["keys"] = copy.deepcopy(v)
                else:
                    w[k] = v
        sim.now = f32(sim.now + f32(st.get("dt", 0.0)))
        sim.frame += 1
        if "event" in st:
            sim.run_event(st["event"])
        else:
            sim.update_fsm(st["mode"])
        trace.append([sim.current, sim.last_event])
    return dict(name=name, start=start, base=wire_world(base_world()), steps=[dict(mode=s.get("mode", 0), event=s.get("event"), dt=bits(s.get("dt", 0.0)), set=_wire_set(s.get("set", {}))) for s in steps],
                trace=trace)


def wire_world(w):
    o = {}
    o.update(_wire_set(w))
    return o


def _wire_set(d):
    o = {}
    for k, v in d.items():
        if k == "keys":
            o[k] = v
        elif k in ("tgt", "ltr", "rbv", "vrb", "lcd", "lcn", "fup"):
            o[k] = b3(v)
        elif k in ("dh", "ldh", "hft", "hfs", "t", "lbs", "spd", "cnt", "cend", "clen"):
            o[k] = bits(v)
        else:
            o[k] = v
    return o


def K(**kw):
    """keys held / down / up"""
    out = {}
    for name, flags in kw.items():
        if isinstance(flags, str):
            flags = (flags,)
        out[name] = {f: True for f in flags}
    return out


F, U, L = 2, 1, 4
FWD = (0.0, 0.0, 1.0)
Z = (0.0, 0.0, 0.0)


def all_scenarios():
    tbl = eva_tables.to_json(eva_tables.kerbal_eva_table())
    out = []
    # 1. walk, stop (arcade frame, 1 g)
    out.append(scenario("walk_stop", "Idle (Grounded)", [
        dict(mode=F, dt=0.02, set=dict(tgt=FWD)),
        dict(mode=F, dt=0.02),
        dict(mode=F, dt=0.02, set=dict(tgt=Z)),
        dict(mode=F, dt=0.02, set=dict(tgt=FWD, cf=True)),
        dict(mode=F, dt=0.02, set=dict(tgt=Z)),
    ], tbl))
    # 2. run: hold Run while walking, release
    out.append(scenario("walk_run", "Idle (Grounded)", [
        dict(mode=F, dt=0.02, set=dict(tgt=FWD)),
        dict(mode=F, dt=0.02, set=dict(keys=K(Run="held"))),
        dict(mode=F, dt=0.02),
        dict(mode=F, dt=0.02, set=dict(keys=K())),
        dict(mode=F, dt=0.02, set=dict(tgt=Z)),
        dict(mode=F, dt=0.02, set=dict(tgt=FWD, cf=True, keys=K(Run="held"))),
        dict(mode=F, dt=0.02, set=dict(keys=K())),
    ], tbl))
    # 3. jump, flight, landing
    out.append(scenario("jump_land", "Idle (Grounded)", [
        dict(mode=U, dt=0.02, set=dict(keys=K(Jump="held"))),
        dict(mode=U, dt=0.1, set=dict(keys=K())),
        dict(mode=U, dt=0.15),
        dict(mode=U, dt=0.1, set=dict(landed=False, hft=2.0)),
        dict(mode=U, dt=0.1),
        dict(mode=F, dt=0.02, set=dict(hft=2.0)),
        dict(mode=U, dt=0.1, set=dict(landed=True, hft=0.0, vrb=(0.0, -1.0, 0.0), lcd=(0.0, -1.0, 0.0), lcn=(0.0, 1.0, 0.0), ut=1000.0, lct=0.0)),
        dict(mode=U, dt=0.02),
        dict(mode=U, dt=0.05),
        dict(mode=U, dt=0.1),
        dict(mode=U, dt=0.1),
    ], tbl))
    # 4. swim
    out.append(scenario("swim", "Idle (Floating)", [
        dict(mode=U, dt=0.02, set=dict(landed=False, splashed=True, hft=0.0)),
        dict(mode=U, dt=0.02),
        dict(mode=U, dt=0.02, set=dict(tgt=FWD)),
        dict(mode=U, dt=0.02, set=dict(tgt=Z)),
        dict(mode=U, dt=0.02, set=dict(landed=True, splashed=False)),
        dict(mode=U, dt=0.02),
    ], tbl))
    # 5. ladder: grab, climb, descend, stop, lean, push off
    out.append(scenario("ladder", "Idle (Floating)", [
        dict(mode=U, dt=0.02, set=dict(landed=False, ntrig=1, clrot=0, hft=3.0)),
        dict(mode=U, dt=0.02, set=dict(keys=K(Use="down"))),
        dict(mode=U, dt=0.5, set=dict(keys=K())),
        dict(mode=U, dt=0.6),
        dict(mode=U, dt=0.02, set=dict(ltr=(0.0, 1.0, 0.0))),
        dict(mode=U, dt=0.02, set=dict(ltr=(0.0, -1.0, 0.0))),
        dict(mode=U, dt=0.02),
        dict(mode=U, dt=0.02, set=dict(ltr=Z)),
        dict(mode=U, dt=0.02, set=dict(keys=K(Run="held"))),
        dict(mode=U, dt=0.02, set=dict(keys=K(Run="held", Jump="held"))),
        dict(mode=F, dt=0.2),
        dict(mode=F, dt=0.2),
        dict(mode=F, dt=0.2),
    ], tbl))
    # 6. ladder: grab and let go; ladder lost
    out.append(scenario("ladder_letgo", "Idle (Grounded)", [
        dict(mode=U, dt=0.02, set=dict(ntrig=1, clrot=0, keys=K(Use="down"))),
        dict(mode=U, dt=1.2, set=dict(keys=K())),
        dict(mode=U, dt=0.02, set=dict(keys=K(Jump="down"))),
        dict(mode=U, dt=0.02, set=dict(keys=K(), landed=False, hft=3.0)),
        dict(mode=U, dt=0.02, set=dict(ntrig=1, keys=K(Use="down"))),
        dict(mode=U, dt=1.2, set=dict(keys=K())),
        dict(mode=F, dt=0.02, set=dict(ntrig=0)),
        dict(mode=F, dt=0.02),
    ], tbl))
    # 7. flag planting
    out.append(scenario("flag_plant", "Idle (Grounded)", [
        dict(event="Flag Plant Started", dt=0.02),
        dict(mode=F, dt=0.02, set=dict(dh=f32(80.0), ldh=f32(80.0))),
        dict(mode=F, dt=0.02, set=dict(dh=f32(5.0), ldh=f32(5.0))),
        dict(mode=U, dt=0.5),
        dict(mode=U, dt=1.5),
        dict(mode=U, dt=1.5),
    ], tbl))
    # 8. stumble, ragdoll, recover
    out.append(scenario("ragdoll_recover", "Idle (Grounded)", [
        dict(event="Stumble", dt=0.02, set=dict(rbv=(0.0, 5.0, 0.0), tgt=Z)),
        # ragdoll_OnEnter calls Vessel.UpdateLandedSplashed (the vessel recomputes its situation): the script re-asserts the ground contact the physics
        # world would report
        dict(mode=F, dt=0.1, set=dict(landed=True)),
        dict(mode=F, dt=0.2, set=dict(rbv=(0.0, 0.1, 0.0), tgt=FWD, landed=True)),
        dict(mode=F, dt=0.2, set=dict(landed=True)),
        dict(mode=U, dt=0.3, set=dict(landed=True)),
        dict(mode=U, dt=1.0, set=dict(landed=True)),
        dict(mode=U, dt=0.02),
    ], tbl))
    # 9. low gravity bounds
    out.append(scenario("low_gravity", "Idle (Grounded)", [
        dict(mode=F, dt=0.02, set=dict(gee=0.1, tgt=FWD)),
        dict(mode=U, dt=0.6),
        dict(mode=U, dt=0.2, set=dict(hfs=0.0)),
        dict(mode=F, dt=0.02, set=dict(landed=False, hft=3.0, hfs=3.0)),
        dict(mode=U, dt=0.6),
        dict(mode=F, dt=0.02, set=dict(tgt=Z)),
        dict(mode=F, dt=0.02),
    ], tbl))
    # 10. construction mode
    out.append(scenario("construction", "Idle (Grounded)", [
        dict(mode=F, dt=0.02, set=dict(inconstr=True)),
        dict(mode=F, dt=0.2),
        dict(mode=F, dt=3.0),
        dict(mode=F, dt=0.02, set=dict(inconstr=False)),
        dict(event="Construction Mode Exited", dt=0.02),
        dict(mode=F, dt=3.0),
        dict(mode=F, dt=0.02),
    ], tbl))
    # 11. pack toggle and boarding (the airlock part): jetpack toggles in Idle (Grounded)
    out.append(scenario("jetpack_toggle", "Idle (Grounded)", [
        dict(mode=U, dt=0.02, set=dict(keys=K(TogglePack="down"))),
        dict(mode=U, dt=0.02, set=dict(keys=K())),
        dict(mode=U, dt=0.02, set=dict(keys=K(TogglePack="down"))),
        dict(mode=U, dt=0.02, set=dict(keys=K(), landed=False, hft=2.0)),
        dict(mode=F, dt=0.02),
        dict(mode=U, dt=0.02, set=dict(keys=K(TogglePack="down"))),
    ], tbl))
    return out
