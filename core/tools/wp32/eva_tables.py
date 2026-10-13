#!/usr/bin/env python3
"""WP-32: the KFSM state / event tables of KerbalEVA.SetupFSM and RetractableLadder.OnStart, PARSED from the decompiled C# (no kspcore code).

KerbalEVA.cs:3268-4352 builds 41 states and 76 events with statements of a handful of shapes; this module reads the text and replays them with
the semantics of KerbalFSM.cs / KFSMState.cs / KFSMEvent.cs (AddState, AddEvent processes the target states last to first, AddEventExcluding walks
the state list last to first, duplicated registrations add twice, a KFSMEvent defaults to update mode UPDATE, a KFSMState to FIXEDUPDATE).
`ExpansionsLoader.IsExpansionInstalled("Serenity")` blocks are taken as installed (the test installs the same assumption).

The result is the independent oracle of the transition table: the C++ port must produce, state by state, the same ordered event lists and callback
names, and event by event the same target state, update mode, timer duration expression and registrations.
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
CS_ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", "decompiled", "KSP-Decompiled", "Assembly-CSharp"))

MODE = {"MANUAL_TRIGGER": 0, "UPDATE": 1, "FIXEDUPDATE": 2, "LATEUPDATE": 4}


def read_lines(name):
    with open(os.path.join(CS_ROOT, name), encoding="utf-8-sig") as f:
        return f.read().split("\n")


def method_body(lines, signature_regex):
    """lines of the method whose declaration matches the regex (brace matching from the first '{' after it)"""
    start = None
    for i, l in enumerate(lines):
        if re.search(signature_regex, l):
            start = i
            break
    if start is None:
        raise RuntimeError("method not found: " + signature_regex)
    depth = 0
    began = False
    out = []
    for i in range(start, len(lines)):
        l = lines[i]
        out.append((i + 1, l))
        depth += l.count("{") - l.count("}")
        if "{" in l:
            began = True
        if began and depth == 0:
            break
    return out


class Fsm:
    def __init__(self):
        self.states = []          # [name]
        self.events_of = {}       # state name -> [event name]
        self.state_info = {}      # name -> dict
        self.event_info = {}      # name -> dict

    def add_state(self, name):
        self.states.append(name)
        self.events_of.setdefault(name, [])

    def add_event(self, ev, to_states):
        for st in reversed(to_states):
            if st not in self.states:
                raise RuntimeError("Cannot add events to state " + st)
            # `States[count] == toStates[num]` for every registered occurrence: one add per matching entry
            for s in self.states:
                if s == st:
                    self.events_of[st].append(ev)

    def add_event_excluding(self, ev, excluded):
        for s in reversed(self.states):
            if s not in excluded:
                self.events_of[s].append(ev)


def parse_fsm(lines, body_regex, state_prefix="st_", event_prefix="On_"):
    body = method_body(lines, body_regex)
    fsm = Fsm()
    var2state = {}   # C# variable -> display name
    var2event = {}
    alias = {}       # kFSMStateN -> st_x
    for no, raw in body:
        l = raw.strip()
        nested = (len(raw) - len(raw.lstrip(chr(9)))) > 3   # inside a delegate body (`OnEvent = delegate { ... }`): runtime assignments, not setup
        m = re.match(r"(st_\w+) = new KFSMState\(\"([^\"]*)\"\);", l)
        if m:
            var2state[m.group(1)] = m.group(2)
            fsm.state_info[m.group(2)] = dict(var=m.group(1), updateMode=MODE["FIXEDUPDATE"], onEnter=None, onLeave=None, onUpdate=[], onFixedUpdate=[],
                                              onLateUpdate=[], line=no)
            continue
        m = re.match(r"(On_\w+) = new (KFSMTimedEvent|KFSMEvent)\(\"([^\"]*)\"(?:, (.+))?\);", l)
        if m:
            var2event[m.group(1)] = m.group(3)
            fsm.event_info[m.group(3)] = dict(var=m.group(1), timed=m.group(2) == "KFSMTimedEvent", duration=m.group(4), goto=None, updateMode=MODE["UPDATE"],
                                              line=no)
            continue
        m = re.match(r"KFSMState (kFSMState\d*) = (st_\w+);", l)
        if m:
            alias[m.group(1)] = m.group(2)
            continue
        # direct or aliased callback assignment
        m = re.match(r"(st_\w+|kFSMState\d*)\.(OnEnter|OnLeave) = (\w+);", l)
        if m:
            v = alias.get(m.group(1), m.group(1))
            name = var2state[v]
            fsm.state_info[name]["on" + m.group(2)[2:]] = m.group(3)
            continue
        m = re.match(r"(st_\w+|kFSMState\d*)\.(OnUpdate|OnFixedUpdate|OnLateUpdate) = (\w+);", l)
        if m:
            v = alias.get(m.group(1), m.group(1))
            name = var2state[v]
            fsm.state_info[name]["o" + m.group(2)[1:]].append(m.group(3))   # plain assignment: replaces; only ever done on an empty list
            continue
        m = re.match(r"(st_\w+|kFSMState\d*)\.(OnUpdate|OnFixedUpdate|OnLateUpdate) = \(KFSMCallback\)Delegate\.Combine\(\1\.\2, new KFSMCallback\((\w+)\)\);", l)
        if m:
            v = alias.get(m.group(1), m.group(1))
            name = var2state[v]
            fsm.state_info[name]["o" + m.group(2)[1:]].append(m.group(3))
            continue
        m = re.match(r"(st_\w+)\.updateMode = KFSMUpdateMode\.(\w+);", l)
        if m:
            fsm.state_info[var2state[m.group(1)]]["updateMode"] = MODE[m.group(2)]
            continue
        if nested:
            continue
        m = re.match(r"(On_\w+)\.GoToStateOnEvent = (st_\w+);", l)
        if m:
            fsm.event_info[var2event[m.group(1)]]["goto"] = var2state[m.group(2)]
            continue
        m = re.match(r"(On_\w+)\.updateMode = KFSMUpdateMode\.(\w+);", l)
        if m:
            # `On_grapple.updateMode = MANUAL_TRIGGER` appears again after On_fully_deploy_parachute / On_parachute_cut (sic): same event, same value
            fsm.event_info[var2event[m.group(1)]]["updateMode"] = MODE[m.group(2)]
            continue
        m = re.match(r"fsm\.AddState\((st_\w+)\);", l)
        if m:
            fsm.add_state(var2state[m.group(1)])
            continue
        m = re.match(r"fsm\.AddEvent\((On_\w+), (.+)\);", l)
        if m:
            fsm.add_event(var2event[m.group(1)], [var2state[s.strip()] for s in m.group(2).split(",")])
            continue
        m = re.match(r"fsm\.AddEventExcluding\((On_\w+), (.+)\);", l)
        if m:
            fsm.add_event_excluding(var2event[m.group(1)], [var2state[s.strip()] for s in m.group(2).split(",")])
            continue
    return fsm


def kerbal_eva_table():
    lines = read_lines("KerbalEVA.cs")
    fsm = parse_fsm(lines, r"protected virtual void SetupFSM\(\)")
    return fsm


def to_json(fsm):
    states = []
    for name in fsm.states:
        si = fsm.state_info[name]
        states.append(dict(name=name, updateMode=si["updateMode"], onEnter=si["onEnter"], onLeave=si["onLeave"], onUpdate=si["onUpdate"],
                           onFixedUpdate=si["onFixedUpdate"], onLateUpdate=si["onLateUpdate"], events=fsm.events_of[name], line=si["line"]))
    events = []
    for name, ei in fsm.event_info.items():
        events.append(dict(name=name, goto=ei["goto"], updateMode=ei["updateMode"], timed=ei["timed"], duration=ei["duration"], line=ei["line"]))
    return dict(states=states, events=events)


def retractable_ladder_table():
    """RetractableLadder.OnStart builds its FSM with `ladderFSM.AddState/AddEvent` and the shapes below (different variable names)."""
    lines = read_lines("RetractableLadder.cs")
    body = method_body(lines, r"public override void OnStart\(StartState state\)")
    fsm = Fsm()
    var2state, var2event = {}, {}
    for no, raw in body:
        l = raw.strip()
        m = re.match(r"(st_\w+) = new KFSMState\(\"([^\"]*)\"\);", l)
        if m:
            var2state[m.group(1)] = m.group(2)
            fsm.state_info[m.group(2)] = dict(var=m.group(1), updateMode=MODE["FIXEDUPDATE"], onEnter=None, onLeave=None, onUpdate=[], onFixedUpdate=[],
                                              onLateUpdate=[], line=no)
            continue
        m = re.match(r"(On_\w+) = new (KFSMTimedEvent|KFSMEvent)\(\"([^\"]*)\"(?:, (.+))?\);", l)
        if m:
            var2event[m.group(1)] = m.group(3)
            fsm.event_info[m.group(3)] = dict(var=m.group(1), timed=m.group(2) == "KFSMTimedEvent", duration=m.group(4), goto=None, updateMode=MODE["UPDATE"],
                                              line=no)
            continue
        m = re.match(r"(On_\w+)\.GoToStateOnEvent = (st_\w+);", l)
        if m:
            fsm.event_info[var2event[m.group(1)]]["goto"] = var2state[m.group(2)]
            continue
        m = re.match(r"(On_\w+)\.updateMode = KFSMUpdateMode\.(\w+);", l)
        if m:
            fsm.event_info[var2event[m.group(1)]]["updateMode"] = MODE[m.group(2)]
            continue
        m = re.match(r"ladderFSM\.AddState\((st_\w+)\);", l)
        if m:
            fsm.add_state(var2state[m.group(1)])
            continue
        m = re.match(r"ladderFSM\.AddEvent\((On_\w+), (.+)\);", l)
        if m:
            fsm.add_event(var2event[m.group(1)], [var2state[s.strip()] for s in m.group(2).split(",")])
            continue
    return fsm


if __name__ == "__main__":
    import json
    t = to_json(kerbal_eva_table())
    print(len(t["states"]), "states", len(t["events"]), "events")
    print(json.dumps(to_json(retractable_ladder_table()), indent=1)[:600])
