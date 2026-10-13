#!/usr/bin/env python3
"""WP-54 independent reference for the Making History mission runtime (flow graph, node Save, scoring).

Written from the decompiled C# (Assembly-CSharp/Expansions/Missions/*) with no kspcore code. It generates the INPUTS (seeded graphs and score
cases) and the EXPECTED outputs; tests/test_missions.cpp rebuilds the same missions in kspcore and compares:

  graphs  random mission graphs (start node, end nodes, catch-all / orphan nodes, event / logic / objective flags, docked nodes, back edges):
          - fromNodes / toNodes / orphan list after Mission.InitMission                       (Mission.cs:401)
          - NodePaths and NodeReversePaths per node                                           (MEFlowParser.cs:231 / :299)
          - the Then / Or block tree of MEFlowParser.ParseMission                              (MEFlowParser.cs:172 ParseBlock :361 ExtractPathGroups :467)
          - IsReachable / IsHiddenByEvent / the root block's four flow properties             (MENode.cs:172 / :199, MEFlowBlock.cs)
          - SetNextObjectives / IsNextObjective                                                (Mission.cs:957 / :986)
          - MissionsUtils.GetNodesOrdered                                                      (MissionsUtils.cs:521)
          - Mission.PrintObjectives for nodes that were never activated (no time text)         (Mission.cs:1037)
          - MENode.Save of every node and MissionFlow.Save                                     (MENode.cs:1148, MissionFlow.cs:40)
          - the validator's per-node "no out connector" failures                               (MENode.cs:943)
  scores  ScoreModule_{Completion,Modifier,Time,Accuracy,Resource}.AwardScore (float32, Mono rule) and the description texts via the real
          Squad/Localization/dictionary.cfg.

Usage: python -I tools/wp54/mission_reference.py   (writes tests/data/wp54_reference.json; reads the GameData mirror's dictionary.cfg)
"""
import json
import math
import os
import random
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CORE = os.path.abspath(os.path.join(HERE, "..", ".."))
DICT = os.path.join(CORE, "testdata", "GameData-mirror", "GameData", "Squad", "Localization", "dictionary.cfg")
OUT = os.path.join(CORE, "tests", "data", "wp54_reference.json")


# ---------------------------------------------------------------------------------------------------------------------------
# number helpers (Mono float rule: operands are floats, arithmetic happens in double, the store rounds to float32)
def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def f32bits(x):
    return "0x%08x" % struct.unpack("<I", struct.pack("<f", x))[0]


def _shortest(x, as_float32):
    from decimal import Decimal

    if x == 0:
        return "0"
    for p in range(1, 18):
        t = "%.*e" % (p - 1, x)
        v = float(t)
        if (f32(v) == f32(x)) if as_float32 else (v == x):
            out = format(Decimal(t), "f")
            if "." in out:
                out = out.rstrip("0").rstrip(".")
            return out
    raise ValueError(x)


def fnum(x):
    """float.ToString() for the test values (shortest round-trip digits, positional notation)."""
    s = _shortest(x, True)
    if len(s.replace("-", "").replace(".", "").lstrip("0")) > 7:
        raise ValueError("more than 7 digits: %r" % x)
    return s


def dnum(x):
    """double.ToString() for the test values (<= 15 digits, positional notation)."""
    s = _shortest(x, False)
    if len(s.replace("-", "").replace(".", "").lstrip("0")) > 15:
        raise ValueError("more than 15 digits: %r" % x)
    return s


def g9(x):
    return "%.9g" % f32(x)


def g17(x):
    return "%.17g" % x


# ---------------------------------------------------------------------------------------------------------------------------
# Localizer (managed Lingoona fallback): <<N>> -> argument N (1-based, text from the first '^' removed), an out of range N is removed;
# arguments that are tags are translated first (Localizer.ReplaceTags); the result is unescaped (\n \" \t).
def load_dictionary():
    tags = {}
    # WP-54c: also the keys the dictionary spells `#autoLoc_` (the C# looks tags up by exact string; "#autoLoc_6002179" is one of the dictionary's own keys)
    pat = re.compile(r"^\s*(#auto[Ll][Oo][Cc]_\d+)\s*=\s*(.*?)\s*$")
    for line in open(DICT, encoding="utf-8-sig"):
        m = pat.match(line)
        if m and m.group(1) not in tags:
            tags[m.group(1)] = m.group(2)
    return tags


TAGS = load_dictionary()


def tag(t):
    return TAGS.get(t, t)


def fmt(template, *args):
    tpl = tag(template)
    if not args:
        out = tpl
    else:
        params = [tag(a) for a in args]

        def sub(m):
            n = int(m.group(1))
            if 1 <= n <= len(params):
                return params[n - 1].split("^")[0]
            return ""

        out = re.sub(r"<<(\d+)(\^[^>]*)?>>", sub, tpl)
    return out.replace("\\n", "\n").replace('\\"', '"').replace("\\t", "\t")


# ---------------------------------------------------------------------------------------------------------------------------
# canonical ConfigNode dump (values first, then child nodes, as ConfigNode.ToString orders them): name{k=v;k=v;child{...}}
class CN:
    def __init__(self, name):
        self.name = name
        self.values = []
        self.nodes = []

    def add(self, k, v):
        self.values.append((k, v))

    def node(self, name):
        n = CN(name)
        self.nodes.append(n)
        return n

    def dump(self):
        s = self.name + "{" + "".join("%s=%s;" % kv for kv in self.values)
        s += "".join(n.dump() for n in self.nodes)
        return s + "}"


def b(x):
    return "True" if x else "False"


# ---------------------------------------------------------------------------------------------------------------------------
# the mission model (only what the graph code reads)
class N:
    pass


def guid(i):
    return "6d650000-0000-0000-0000-%012d" % (i + 1)


class Mission:
    def __init__(self, spec):
        self.nodes = []
        for i, s in enumerate(spec["nodes"]):
            n = N()
            n.idx = i
            n.id = guid(i)
            n.title = s["title"]
            n.is_start = s["start"]
            n.is_end = s["end"]
            n.is_catch = s["catch"]
            n.is_obj = s["obj"]
            n.is_event = s["event"]
            n.is_logic = s["logic"]
            n.activated_ut = s["activatedUT"]
            n.from_ids = list(s["from"])
            n.to_ids = list(s["to"])
            n.dock_parent_idx = s["dockParent"]
            n.docked_idx = list(s["docked"])
            n.editor_pos = s["pos"]
            n.msg_time = s["msgTime"]
            n.description = s["description"]
            n.message = s["message"]
            n.once = s["once"]
            n.show_msg = s["showMsg"]
            n.end_opt = s["endOpt"]
            n.docked_index = s["dockedIndex"]
            n.category = s["category"]
            if n.category == "" and n.is_start:  # MENode.Load :1033
                n.category = "Start"
            n.basic_src = s["basicSrc"]
            n.from_nodes = []
            n.to_nodes = []
            n.dock_parent = None
            n.docked = []
            self.nodes.append(n)
        self.start = None
        for n in self.nodes:
            if n.is_start:
                self.start = n
                break
        self.active = self.nodes[spec["active"]] if spec["active"] >= 0 else self.start
        self.started = spec["started"]
        # Mission.Load -> SetDockedNodes (:569) runs before InitMission
        for n in self.nodes:
            if n.dock_parent_idx >= 0:
                n.dock_parent = self.nodes[n.dock_parent_idx]
            for j in n.docked_idx:
                if self.nodes[j] not in n.docked:
                    n.docked.append(self.nodes[j])
        self.orphans = []
        self.node_paths = {}  # insertion ordered
        self.node_rev = {}
        self.block = None
        self.next_obj = None

    # MENode properties
    def is_docked(self, n):
        return n.dock_parent is not None

    def is_docked_to_start(self, n):
        return n.dock_parent is not None and n.dock_parent.id == self.start.id

    def is_orphan(self, n):
        return len(n.from_ids) < 1 and not n.is_start and n.is_catch and not self.is_docked(n)

    def has_been_activated(self, n):
        return n.activated_ut > 0.0 or n.is_start or self.is_docked_to_start(n)

    def is_active(self, n):
        return self.active is n

    def is_reachable(self, n):
        if self.started and n in self.node_rev:
            return any(self.active in p for p in self.node_rev[n])
        return True

    def is_hidden_by_event(self, n):
        if n in self.node_rev:
            for p in self.node_rev[n]:
                for m in p:
                    if not self.is_active(m):
                        if m.is_event and not self.has_been_activated(m):
                            return True
                        continue
                    return False
        return False

    def has_connected_output(self, n):
        if len(n.to_nodes) < 1:
            return False
        return any(n.id in [self.nodes[i].id for i in t.from_ids] for t in n.to_nodes)

    # Mission.InitMission :401
    def init_mission(self):
        self.orphans = []
        for n in self.nodes:
            n.from_nodes = []
            if len(n.from_ids) > 0:
                for i in n.from_ids:
                    n.from_nodes.append(self.nodes[i])
            elif self.is_orphan(n):
                self.orphans.append(n)
        for n in self.nodes:
            if len(n.to_ids) <= 0:
                continue
            for j in n.to_ids:
                if n in self.nodes[j].from_nodes:
                    if self.nodes[j] not in n.to_nodes:
                        n.to_nodes.append(self.nodes[j])
            n.to_ids = []
        for n in self.nodes:
            for f in n.from_nodes:
                if n not in f.to_nodes:
                    f.to_nodes.append(n)
            if not n.is_end:
                n.to_nodes.extend(self.orphans)
                if self.is_orphan(n):
                    if n in n.to_nodes:
                        n.to_nodes.remove(n)
        for n in self.nodes:
            if n in n.to_nodes:
                n.to_nodes.remove(n)

    # MEFlowParser.BuildPathsByDFS :231
    def build_paths(self, start, visited):
        if start in self.node_paths:
            return self.node_paths[start]
        if visited is None:
            visited = set()
        visited.add(start)
        paths = []
        for m in start.to_nodes:
            if self.is_orphan(m):
                continue
            if m in self.node_paths:
                lst = self.node_paths[m]
            elif m not in visited:
                lst = self.build_paths(m, visited)
            else:
                lst = []
            if len(lst) < 1:
                paths.append([m])
                continue
            for p in lst:
                q = [m] + list(p)
                if len(q) > 1 and q[0] is q[-1]:
                    q.pop()
                paths.append(q)
        if not start.is_start:
            for m in start.docked:
                if not m.is_logic or self.is_orphan(m):
                    continue
                if m in self.node_paths:
                    lst = self.node_paths[m]
                elif m not in visited:
                    lst = self.build_paths(m, visited)
                else:
                    lst = []
                if len(lst) < 1:
                    paths.append([m])
                    continue
                for p in lst:
                    q = [m] + list(p)
                    if len(q) > 1 and q[0] is q[-1]:
                        q.pop()
                    paths.append(q)
        self.node_paths[start] = paths
        return paths

    # MEFlowParser.BuildReversePathsByDFS :299
    def build_rev(self, start, visited):
        if start in self.node_rev:
            return self.node_rev[start]
        if visited is None:
            visited = set()
        visited.add(start)
        paths = []
        if self.is_orphan(start):
            return paths
        for m in start.from_nodes:
            if m in self.node_rev:
                lst = self.node_rev[m]
            elif m not in visited:
                lst = self.build_rev(m, visited)
            else:
                lst = []
            if len(lst) < 1:
                paths.append([m])
                continue
            for p in lst:
                q = [m] + list(p)
                if len(q) > 1 and q[0] is q[-1]:
                    q.pop()
                paths.append(q)
        dp = start.dock_parent
        if dp is not None:
            if dp in self.node_rev:
                lst = self.node_rev[dp]
            elif dp not in visited:
                lst = self.build_rev(dp, visited)
            else:
                lst = []
            if len(lst) < 1:
                paths.append([dp])
            else:
                for p in lst:
                    q = [dp] + list(p)
                    if len(q) > 1 and q[0] is q[-1]:
                        q.pop()
                    paths.append(q)
        self.node_rev[start] = paths
        return paths

    # MEFlowParser.ParseMission :172
    def parse(self):
        self.node_paths = {}
        self.node_rev = {}
        self.block = ("B", [])  # the MissionFlow constructor's plain MEFlowBlock
        if self.start is None:
            return
        self.build_paths(self.start, None)
        ends = []
        for n in self.nodes:
            if self.is_docked_to_start(n):
                continue
            if n.is_end:
                ends.append(n)
                continue
            if len(n.to_nodes) < 1:
                ends.append(n)
                continue
            if not any(not self.is_orphan(t) for t in n.to_nodes):
                ends.append(n)
        for e in ends:
            self.build_rev(e, None)
        paths = [list(p) for p in self.node_paths[self.start]]
        if len(paths) > 0:
            self.block = parse_block(paths, None)


# MEFlowParser.ParseBlock :361 / ExtractPathGroups :467 over plain lists of node lists (paths are mutated in place like the C#)
def distinct_starts(paths):
    out = []
    for p in paths:
        if len(p) > 0 and p[0] not in out:
            out.append(p[0])
    return out


def get_convergence(path, target, min_hops):
    for i in range(min_hops, len(path)):
        if path[i] in target:
            return (path[i], i)
    return None


def extract_groups(paths):
    groups = []  # each: dict(paths, join, minHops)
    i = 0
    while i < len(paths):
        p = paths[i]
        flag = False
        for g in groups:
            if g["paths"][0] is p:
                flag = True
            else:
                c = get_convergence(g["paths"][0], p, g["minHops"])
                if c is not None:
                    flag = True
                    g["paths"].append(p)
                    if g["join"] is None:
                        g["join"] = c[0]
                    else:
                        first = g["paths"][0]
                        idx = first.index(g["join"]) if g["join"] in first else -1
                        if idx < c[1]:
                            g["join"] = c[0]
                            g["minHops"] = c[1]
                            del g["paths"][1:]
                            i = -1
                    break
        if not flag:
            groups.append({"paths": [p], "join": None, "minHops": 0})
        i += 1
    return groups


def parse_block(paths, insert):
    if len(paths) >= 1 and len(paths[0]) > 0:
        block = ("T", [])
        if insert is not None:
            block[1].append(insert)
        flag = False
        while not flag:
            if len(distinct_starts(paths)) == 1:
                while len(distinct_starts(paths)) == 1:
                    if len(paths[0]) < 1:
                        del paths[0]
                        continue
                    block[1].append(paths[0][0])
                    for p in paths:
                        if len(p) > 0:
                            del p[0]
            lst = []
            if len(distinct_starts(paths)) > 1:
                groups = extract_groups(paths)
                if len(paths[0]) > 0:
                    orb = ("O", [])
                    if len(groups) > 1:
                        for g in groups:
                            sub = parse_block(g["paths"], None)
                            if sub is not None:
                                orb[1].append(sub)
                    else:
                        paths2 = groups[0]["paths"]
                        for p in paths2:
                            if not (len(p) > 0):
                                continue
                            p2 = []
                            count = len(p)
                            while count > 0:
                                count -= 1
                                num = p.index(groups[0]["join"]) if groups[0]["join"] in p else -1
                                if num < 0:
                                    break
                                if num <= count:
                                    p2.insert(0, p[count])
                                    del p[count]
                            lst.append(p2)
                        for s in distinct_starts(paths2):
                            starting = [p for p in paths2 if len(p) > 0 and p[0] is s]
                            sub = parse_block(starting, None)
                            if sub is not None:
                                orb[1].append(sub)
                    if len(orb[1]) > 0:
                        block[1].append(orb)
            if len(lst) > 0:
                paths = lst
            else:
                flag = True
        return block
    return None


def block_str(b_):
    if isinstance(b_, N):
        return str(b_.idx)
    kind, kids = b_
    return kind + "(" + " ".join(block_str(k) for k in kids) + ")"


def block_save(b_, node):
    """MEFlowBlock.Save :139"""
    for k in b_[1]:
        if isinstance(k, N):
            node.node("NODE").add("id", k.id)
        elif k[0] == "T":
            block_save(k, node.node("THEN"))
        elif k[0] == "O":
            block_save(k, node.node("OR"))


def b_count_objectives(b_):
    n = 0
    for k in b_[1]:
        if isinstance(k, N):
            if k.is_obj:
                n += 1
        elif k[0] in ("O", "T") and b_count_objectives(k) > 0:
            n += 1
    return n


def b_reachable(m, b_):
    for k in b_[1]:
        if isinstance(k, N):
            if m.is_reachable(k):
                return True
        elif k[0] in ("O", "T") and b_reachable(m, k):
            return True
    return False


def b_visible(m, b_):
    for k in b_[1]:
        if isinstance(k, N):
            if not m.is_hidden_by_event(k):
                return True
        elif k[0] in ("O", "T") and b_visible(m, k):
            return True
    return False


def hex2(v):
    return "%02X" % v


def dump_node(m, n):
    """MENode.Save :1148 for a node without test groups / actions."""
    c = CN("NODE")
    c.add("title", n.title)
    c.add("id", n.id)
    c.add("description", n.description.replace("\n", "\\n").replace("\t", "\\t"))
    if n.category:
        c.add("category", n.category)
    c.add("showScreenMessage", b(n.show_msg))
    c.add("msgTime", str(n.msg_time))
    c.add("editorPosition", g9(n.editor_pos[0]) + "," + g9(n.editor_pos[1]))
    c.add("isStartNode", b(n.is_start))
    c.add("isEndNode", b(n.is_end))
    c.add("message", n.message.replace("\n", "\\n").replace("\t", "\\t"))
    c.add("dockedIndex", str(n.docked_index))
    c.add("isObjective", b(n.is_obj))
    c.add("isCatchAll", b(n.is_catch))
    c.add("isEvent", b(n.is_event))
    c.add("activateOnceOnly", b(n.once))
    c.add("activatedUT", g17(n.activated_ut))
    c.add("isLogicNode", b(n.is_logic))
    c.add("basicNodeSource", n.basic_src)
    c.add("basicNodeIconURL", "")
    c.add("nodeColor", "#808080")
    if n.dock_parent is not None:
        c.add("dockParentNodeID", n.dock_parent.id)
    if len(n.from_nodes) > 0:
        fn = c.node("FROMNODES")
        for f in n.from_nodes:
            fn.node("FROMNODE").add("nodeID", f.id)
    if len(n.to_nodes) > 0:
        tn = c.node("TONODEIDS")
        for t in n.to_nodes:
            if not m.is_orphan(t):
                tn.add("nodeID", t.id)
    if len(n.docked) > 0:
        dn = c.node("DOCKEDNODEIDS")
        for d in n.docked:
            dn.add("nodeID", d.id)
    c.add("missionEndOptions", n.end_opt)
    return c.dump()


def objectives_text(m, only_activated, start_with_active):
    """Mission.PrintObjectives :1037 (never activated nodes only: no time suffix)."""
    out = []
    visited = []

    def rec(cur, indent, reject):
        if cur is None or (only_activated and not cur.is_start and not m.has_been_activated(cur)) or (reject and m.is_orphan(cur)):
            return
        if cur.is_obj:
            assert not m.has_been_activated(cur), "the reference prints no time text"
            out.append(indent + "<color=#EDFEAAFF>\u2022 " + cur.title + "</color> \n")
            indent += "\t"
        visited.append(cur)
        for t in cur.to_nodes:
            if t not in visited:
                rec(t, indent, True)

    if start_with_active:
        visited.append(m.active)
        for t in m.active.to_nodes:
            rec(t, "", True)
    else:
        rec(m.start, "", True)
    for n in m.nodes:
        if len(n.from_ids) == 0 and not n.is_start:
            rec(n, "", False)
    s = "".join(out)
    return s if s != "" else "#autoLOC_8100310"


def nodes_ordered(m, objectives_only):
    """MissionsUtils.GetNodesOrdered :521 (Graph + BreadthSearch :58)."""
    adj = {}
    for n in m.nodes:
        adj[n] = []
    for n in m.nodes:
        for f in n.from_nodes:
            if f in adj and n in adj:
                if n not in adj[f]:
                    adj[f].append(n)
    seen = []
    if m.start not in adj:
        return []
    q = [m.start]
    while q:
        n = q.pop(0)
        if n in seen:
            continue
        seen.append(n)
        for c in adj[n]:
            if c not in seen:
                q.append(c)
    return [n.idx for n in seen if (not objectives_only or n.is_obj)]


# ---------------------------------------------------------------------------------------------------------------------------
def gen_graph(rng, gi):
    n = rng.randint(3, 15)
    nodes = []
    for i in range(n):
        s = dict(
            title="Node %d" % i,
            start=(i == 0),
            end=False,
            catch=False,
            obj=rng.random() < 0.5,
            event=rng.random() < 0.15,
            logic=rng.random() < 0.15,
            activatedUT=0.0,
            dockParent=-1,
            docked=[],
            description="",
            message="",
            msgTime=5,
            once=True,
            showMsg=False,
            endOpt="Success",
            dockedIndex=0,
            category="",
            basicSrc="",
        )
        s["from"] = []
        s["to"] = []
        s["pos"] = [rng.randint(-40, 40) * 0.5, rng.randint(-40, 40) * 0.25]
        if i > 0:
            r = rng.random()
            s["end"] = r < 0.12
            s["catch"] = 0.12 <= r < 0.2
        if rng.random() < 0.2:
            s["description"] = "line one\nline\ttwo"
            s["message"] = "msg %d\tx" % i
        if rng.random() < 0.2:
            s["msgTime"] = rng.randint(2, 20)
            s["showMsg"] = True
            s["once"] = rng.random() < 0.5
            s["endOpt"] = "Fail" if rng.random() < 0.5 else "Success"
            s["category"] = rng.choice(["", "Start", "Logic", "Vessel"])
            s["basicSrc"] = rng.choice(["", "TestBasic", "ActionBasic"])
        nodes.append(s)
    nodes[0]["obj"] = False
    nodes[0]["end"] = False
    nodes[0]["catch"] = False
    nodes[0]["logic"] = False
    nodes[0]["event"] = False
    # edges: forward edges, a few back edges; catch-all nodes get no incoming edge
    edges = set()
    for i in range(n):
        for j in range(n):
            if i == j or nodes[i]["end"] or j == 0:
                continue
            if nodes[j]["catch"]:
                continue
            p = 0.28 if j > i else 0.04
            if j == i + 1:
                p = 0.7
            if rng.random() < p:
                edges.add((i, j))
    if not any(e[0] == 0 for e in edges) and n > 1:
        for j in range(1, n):
            if not nodes[j]["catch"]:
                edges.add((0, j))
                break
    # docked nodes: logic or non-logic nodes docked to the start node or to another node (never to themselves)
    for j in range(1, n):
        if nodes[j]["catch"] or rng.random() >= 0.18:
            continue
        parent = 0 if rng.random() < 0.6 else rng.randint(0, n - 1)
        if parent == j:
            parent = 0
        # a docked node has no incoming edges in a real mission
        for (a, c) in list(edges):
            if c == j:
                edges.discard((a, c))
        nodes[j]["dockParent"] = parent
        nodes[parent]["docked"].append(j)
        if rng.random() < 0.5:
            nodes[j]["catch"] = True
        nodes[j]["dockedIndex"] = len(nodes[parent]["docked"])
    for (a, c) in sorted(edges):
        nodes[c]["from"].append(a)
        # TONODEIDS of the saved file: the target's id; inconsistent entries (no matching from) appear in 8% of the edges
        nodes[a]["to"].append(c)
    # some inconsistent toNodeIDs entries (the target does not list the source in its FROMNODES): InitMission ignores them
    for _ in range(rng.randint(0, 2)):
        a, c = rng.randint(0, n - 1), rng.randint(1, n - 1)
        if (a, c) not in edges and a != c:
            nodes[a]["to"].append(c)
    # activation state: the start node and some others
    active = rng.randint(0, n - 1)
    started = rng.random() < 0.7
    if started:
        for i in range(1, n):
            if rng.random() < 0.25:
                nodes[i]["activatedUT"] = float(rng.randint(1, 400)) * 0.5
    return dict(nodes=nodes, active=active, started=started)


def expected_for_graph(spec):
    m = Mission(spec)
    m.init_mission()
    m.parse()
    exp = {}
    exp["fromNodes"] = [[f.idx for f in n.from_nodes] for n in m.nodes]
    exp["toNodes"] = [[t.idx for t in n.to_nodes] for n in m.nodes]
    exp["orphans"] = [o.idx for o in m.orphans]
    exp["paths"] = [[n.idx, [[x.idx for x in p] for p in ps]] for n, ps in m.node_paths.items()]
    exp["revPaths"] = [[n.idx, [[x.idx for x in p] for p in ps]] for n, ps in m.node_rev.items()]
    exp["block"] = block_str(m.block)
    exp["isReachable"] = [m.is_reachable(n) for n in m.nodes]
    exp["isHidden"] = [m.is_hidden_by_event(n) for n in m.nodes]
    exp["isOrphan"] = [m.is_orphan(n) for n in m.nodes]
    exp["hasBeenActivated"] = [m.has_been_activated(n) for n in m.nodes]
    exp["isDockedToStart"] = [m.is_docked_to_start(n) for n in m.nodes]
    if m.block[0] != "N":
        exp["blockProps"] = dict(
            countObjectives=b_count_objectives(m.block),
            hasObjectives=b_count_objectives(m.block) > 0,
            hasReachable=b_reachable(m, m.block),
            hasVisible=b_visible(m, m.block),
        )
    # SetNextObjectives(active) :957 (called by Mission.Load when a FLOW node exists; the test calls it through ActivateNode(Loading))
    nxt = []
    if m.active in m.node_paths:
        for p in m.node_paths[m.active]:
            j = 0
            while j < len(p) and (not p[j].is_event or m.has_been_activated(p[j])):
                if p[j].is_obj:
                    if p[j] not in nxt:
                        nxt.append(p[j])
                    break
                j += 1
    exp["nextObjectives"] = [n.idx for n in nxt]
    exp["nodesOrdered"] = nodes_ordered(m, False)
    exp["nodesOrderedObjectives"] = nodes_ordered(m, True)
    # PrintObjectives only for graphs without activated objective nodes (no time suffix in the reference)
    printable = not any(m.has_been_activated(n) and n.is_obj for n in m.nodes)
    exp["printable"] = printable
    if printable:
        exp["printAll"] = objectives_text(m, False, False)
        exp["printActivatedOnly"] = objectives_text(m, True, False)
        exp["printFromActive"] = objectives_text(m, False, True)
    exp["nodeSave"] = [dump_node(m, n) for n in m.nodes]
    flow = CN("FLOW")
    blocks = flow.node("BLOCKS")
    block_save(m.block, blocks)
    exp["flowSave"] = flow.dump()
    # MENode.RunValidationWrapper :943 "no out connector" failures (no launch-site actions / scoring actions in the generated graphs)
    fails = []
    for n in m.nodes:
        if n.is_start:
            continue
        if not n.is_end and not m.is_docked_to_start(n) and not (m.is_docked(n) and False) and not m.has_connected_output(n):
            fails.append(n.idx)
    exp["validatorFails"] = fails
    return exp


# ---------------------------------------------------------------------------------------------------------------------------
def gen_scores(rng):
    cases = []
    for _ in range(60):  # Completion
        cur = f32(rng.choice([0, 1, 10, 100, 250.5, 1234.25, -7.5, 0.125]))
        bonus = f32(rng.choice([0, 5, 10.5, 250, -3, 0.25, 1000.75]))
        res = f32(cur + bonus)
        cases.append(dict(type="Completion", bonus=bonus, current=cur, result=f32bits(res), awarded=fmt("#autoLOC_8006057", fnum(bonus)),
                          desc=fmt("#autoLOC_8100149", tag("#autoLOC_8100148"), fnum(bonus))))
    for typ in ("Multiply", "Divide", "Substract", "Set"):
        for _ in range(30):
            cur = f32(rng.choice([0, 1, 10, 100, 250.5, 1234.25, -7.5, 0.125, 3]))
            mod = f32(rng.choice([2, 0.5, 3.25, 10, 100, -4, 0.125, 7]))
            if typ == "Multiply":
                res = f32(cur * mod)
                awarded = fmt("#autoLOC_8006052", fnum(mod))
            elif typ == "Divide":
                res = f32(cur / mod)
                awarded = fmt("#autoLOC_8006053", fnum(mod))
            elif typ == "Substract":
                res = f32(cur - mod)
                awarded = fmt("#autoLOC_8006054", fnum(mod))
            else:
                res = mod
                awarded = fmt("#autoLOC_8006055", fnum(mod))
            name = "#autoLOC_8100125"
            if typ in ("Multiply", "Divide"):
                desc = fmt("#autoLOC_8100154", name, {"Multiply": "#autoLOC_8100127", "Divide": "#autoLOC_8100128"}[typ], fnum(mod))
            elif typ == "Substract":
                text = tag("#autoLOC_8005414") + " " + fnum(mod) + " " + tag("#autoLOC_8100147")
                desc = fmt("#autoLOC_8004190", name, text)
            else:
                desc = fmt("#autoLOC_8006050", name, fnum(mod))
            cases.append(dict(type="Modifier", modType=typ, mod=mod, current=cur, result=f32bits(res), awarded=awarded, desc=desc))
    # range based modules
    def ranges():
        out = []
        lo = rng.choice([0, 10, 25])
        for _ in range(rng.randint(1, 4)):
            hi = lo + rng.choice([5, 10, 20, 25.5])
            out.append(dict(min=float(lo), max=float(hi), score=f32(rng.choice([1, 5, 10, 25, 50.5, 100]))))
            lo = hi + rng.choice([0, 0.5, 1])
        return out

    def in_range(rs, v):
        for r in rs:
            if r["min"] <= v <= r["max"]:
                return r
        return None

    for _ in range(60):  # Time (AwardScore only: ScoreDescription needs the date formatter)
        rs = ranges()
        ut = rng.choice([0.0, 3.0, 12.5, 30.0, 47.5, 70.0, 200.0, 1000.0])
        cur = f32(rng.choice([0, 10, 100.5]))
        hit = in_range(rs, ut)
        if hit:
            res = f32(cur + hit["score"])
            awarded = fmt("#autoLOC_8001003", fnum(hit["score"]))
        else:
            res = cur
            awarded = ""
        cases.append(dict(type="Time", ranges=rs, ut=ut, current=cur, result=f32bits(res), awarded=awarded))
    for _ in range(80):  # Accuracy
        rs = ranges()
        acc = rng.choice([0.0, 0.0731, 0.1234, 0.2501, 0.3333, 0.4568, 0.7861, 0.9921, 1.0])
        value = acc * 100.0
        cur = f32(rng.choice([0, 10, 100.5]))
        hit = in_range(rs, value)
        if hit:
            res = f32(cur + hit["score"])
            awarded = fmt("#autoLOC_8006056", "%02d" % int(math.floor(value + 0.5)), fnum(hit["score"]))
        else:
            res = cur
            awarded = ""
        desc = fmt("#autoLOC_8100157", tag("#autoLOC_8001002"))
        for r in rs:
            desc += fmt("#autoLOC_8100156", dnum(r["min"]), dnum(r["max"]), fnum(r["score"]))
        cases.append(dict(type="Accuracy", ranges=rs, accuracy=acc, current=cur, result=f32bits(res), awarded=awarded, desc=desc))
    for _ in range(80):  # Resource
        rs = ranges()
        amount = rng.choice([0.0, 4.0, 12.3456, 27.5, 55.25, 100.0, 400.75])
        cur = f32(rng.choice([0, 10, 100.5]))
        hit = in_range(rs, amount)
        if hit:
            res = f32(cur + hit["score"])
            awarded = fmt("#autoLOC_8006058", "%02d" % int(math.floor(amount + 0.5)), fnum(hit["score"]))
        else:
            res = cur
            awarded = ""
        desc = fmt("#autoLOC_8100157", tag("#autoLOC_8100133"))
        for r in rs:
            desc += fmt("#autoLOC_8100160", dnum(r["min"]), dnum(r["max"]), fnum(r["score"]))
        cases.append(dict(type="Resource", ranges=rs, amount=amount, current=cur, result=f32bits(res), awarded=awarded, desc=desc))
    return cases


def main():
    rng = random.Random(54)
    graphs = []
    for gi in range(220):
        spec = gen_graph(rng, gi)
        exp = expected_for_graph(spec)
        graphs.append(dict(spec=spec, expected=exp))
    scores = gen_scores(random.Random(5454))
    out = dict(graphs=graphs, scores=scores,
               dictionary_tags=sorted(t for t in TAGS if t in {"#autoLOC_8100310", "#autoLOC_8006040", "#autoLOC_8006049"}))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(out, f, ensure_ascii=False, separators=(",", ":"))
    print("graphs", len(graphs), "scores", len(scores), "->", OUT, os.path.getsize(OUT), "bytes")
    # statistics for the report
    kinds = {}
    for g in graphs:
        for ch in g["expected"]["block"]:
            if ch in "TO":
                kinds[ch] = kinds.get(ch, 0) + 1
    print("blocks", kinds, "printable", sum(1 for g in graphs if g["expected"]["printable"]))


if __name__ == "__main__":
    main()
