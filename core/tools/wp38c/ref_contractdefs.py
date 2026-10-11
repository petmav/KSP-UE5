"""WP-38 slice 3 reference, part 1: FinePrint.ContractDefs.

Independent of kspcore and of the C++ generator (tools/wp38c/gen_contract_defs.py): it reads FinePrint/ContractDefs.cs itself and
  * collects every static member with its initialiser (the defaults),
  * interprets CreateDefaultConfig / AddSurveyDefinitions / AddPartRequests / AddResourceRequests / AddCrewRequests as a tree builder and renders
    the ConfigNode.Save text (float "G9", double "G17", bool True / False, null string -> "" with the logged error),
  * interprets LoadConfig against a parsed cfg file (SystemUtilities.LoadNode semantics) to give the value of every member after the load,
  * gives the (section, sub, key) -> member path list LoadConfig implies.
Floats are exact decimal -> float32 conversions (no double rounding).
"""
import math
import re
import struct
from fractions import Fraction

import numpy as np

STR = r'"(?:[^"\\]|\\.)*"'


def f32_from_decimal(txt):
    """nearest float32 to the exact decimal txt (ties to even), as numpy.float32"""
    d = Fraction(txt)
    x = np.float32(float(d))
    best = x
    cands = [np.nextafter(x, np.float32(-np.inf)), x, np.nextafter(x, np.float32(np.inf))]
    bestd = None
    for c in cands:
        if not np.isfinite(c):
            continue
        e = abs(Fraction(float(c)) - d)
        if bestd is None or e < bestd or (e == bestd and (struct.unpack("<I", struct.pack("<f", float(c)))[0] & 1) == 0):
            best, bestd = c, e
    return best


def bits32(x):
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def bits64(x):
    return struct.unpack("<Q", struct.pack("<d", float(x)))[0]


def g9(x):
    s = "%.9g" % float(np.float32(x))
    return s.replace("e", "E")


def g17(x):
    s = "%.17g" % float(x)
    return s.replace("e", "E")


def unq(s):
    return bytes(s[1:-1], "utf-8").decode("unicode_escape")


class CS:
    def __init__(self, path):
        self.lines = [l.rstrip("\r") for l in open(path, encoding="utf-8-sig").read().split("\n")]

    def find(self, sig):
        for i, l in enumerate(self.lines):
            if sig in l:
                return i
        raise SystemExit("missing " + sig)

    def body(self, sig):
        i = self.find(sig)
        base = self.lines[i][: len(self.lines[i]) - len(self.lines[i].lstrip("\t"))]
        j = i + 1
        while self.lines[j] != base + "}":
            j += 1
        return i + 1, [l.strip() for l in self.lines[i + 2: j] if l.strip()]


def parse_defaults(cs):
    """path -> (ctype, value) ; float: np.float32, double: float, int, bool, string (None -> null)"""
    out = {}
    start = cs.find("public class ContractDefs : MonoBehaviour") + 1
    end = cs.find("public static PreBuiltCraft PreBuiltCraftDefs;")
    stack = []
    for l in cs.lines[start:end]:
        s = l.strip()
        m = re.match(r"public static class (\w+)$", s)
        if m:
            stack.append(m.group(1))
            continue
        if s == "}":
            if stack:
                stack.pop()
            continue
        m = re.match(r"public static (int|float|bool|double|string) (\w+)(?: = (.+))?;$", s)
        if not m:
            continue
        t, name, init = m.groups()
        if t == "int":
            v = int(init) if init is not None else 0
        elif t == "float":
            v = f32_from_decimal(init.rstrip("f")) if init is not None else np.float32(0)
        elif t == "double":
            v = float(Fraction(init)) if init is not None else 0.0
        elif t == "bool":
            v = (init == "true") if init is not None else False
        else:
            v = unq(init) if init is not None else None
        out[".".join(stack + [name])] = (t, v)
    return out


class Node:
    def __init__(self, name, comment=None):
        self.name, self.comment, self.values, self.nodes = name, comment, [], []

    def add_node(self, n):
        self.nodes.append(n)
        return n


def render(node, depth, out):
    ind = "\t" * depth
    out.append(ind + node.name + ((" // " + node.comment) if node.comment is not None else ""))
    out.append(ind + "{")
    for k, v, c in node.values:
        out.append(ind + "\t" + k + " = " + v + ((" // " + c) if c is not None else ""))
    for n in node.nodes:
        render(n, depth + 1, out)
    out.append(ind + "}")


def save_text(root):
    """ConfigNode.Save of a wrapper node: its values and child nodes without the wrapper, CRLF"""
    out = []
    for k, v, c in root.values:
        out.append(k + " = " + v + ((" // " + c) if c is not None else ""))
    for n in root.nodes:
        render(n, 0, out)
    return "\r\n".join(out) + "\r\n"


def value_text(expr, defaults, serenity=None):
    expr = re.sub(r"^value: ", "", expr.strip())
    if re.fullmatch(STR, expr):
        return unq(expr)
    if expr in ("true", "false"):
        return "True" if expr == "true" else "False"
    if re.fullmatch(r"-?[0-9]+", expr):
        return expr
    if re.fullmatch(r"-?[0-9.]+f", expr):
        return g9(f32_from_decimal(expr[:-1]))
    if re.fullmatch(r"-?[0-9]+\.[0-9]+", expr):
        return g17(float(Fraction(expr)))
    t, v = defaults[expr]
    if t == "int":
        return str(v)
    if t == "bool":
        return "True" if v else "False"
    if t == "float":
        return g9(v)
    if t == "double":
        return g17(v)
    return v if v is not None else ""


def build_tree(cs, defaults):
    """interpret CreateDefaultConfig + the Add* functions -> the Contracts node"""
    env = {}
    wrapper = Node("")
    helpers = {
        "AddSurveyDefinitions": cs.body("private static void AddSurveyDefinitions(ConfigNode surveyNode)")[1],
        "AddPartRequests": cs.body("private static void AddPartRequests(ConfigNode topNode)")[1],
        "AddResourceRequests": cs.body("private static void AddResourceRequests(ConfigNode topNode)")[1],
        "AddCrewRequests": cs.body("private static void AddCrewRequests(ConfigNode topNode)")[1],
    }

    def run(stmts, env, skip_if_null=False):
        i = 0
        skip = []  # stack of booleans: inside a skipped if-block
        pending_if = None
        for s in stmts:
            if any(skip):
                if s == "{":
                    skip.append(True)
                elif s == "}":
                    skip.pop()
                continue
            if pending_if is not None:
                if s == "{":
                    skip.append(not pending_if)
                    pending_if = None
                    continue
            m = re.fullmatch(r"if \((\w+) != null\)", s)
            if m:
                pending_if = env.get(m.group(1)) is not None
                continue
            if s == "{":
                skip.append(False)
                continue
            if s == "}":
                skip.pop()
                continue
            m = re.fullmatch(r"config = new ConfigNode\(\);", s)
            if m:
                continue
            m = re.fullmatch(r"(?:ConfigNode )?(\w+) = (?:(\w+)\.AddNode\(new ConfigNode\((%s)(?:, (%s))?\)\)|(\w+)\.AddNode\((%s)(?:, (%s))?\)|new ConfigNode\((%s)(?:, (%s))?\)|(\w+)\.GetNode\((%s)\));" % ((STR,) * 7), s)
            if m:
                g = m.groups()
                var = g[0]
                if g[1] is not None:
                    parent = wrapper if g[1] == "config" else env[g[1]]
                    n = Node(unq(g[2]), unq(g[3]) if g[3] is not None else None)
                    parent.add_node(n)
                elif g[4] is not None:
                    n = Node(unq(g[5]), unq(g[6]) if g[6] is not None else None)
                    env[g[4]].add_node(n)
                elif g[7] is not None:
                    n = Node(unq(g[7]), unq(g[8]) if g[8] is not None else None)
                else:
                    n = next((x for x in env[g[9]].nodes if x.name == unq(g[10])), None)
                env[var] = n
                continue
            m = re.fullmatch(r"(\w+)\.AddNode\((\w+)\);", s)
            if m:
                env[m.group(1)].add_node(env[m.group(2)])
                continue
            m = re.fullmatch(r"(\w+)\.AddValue\((%s), (.+), (%s)\);" % (STR, STR), s)
            if m:
                env[m.group(1)].values.append((unq(m.group(2)), value_text(m.group(3), defaults), unq(m.group(4))))
                continue
            m = re.fullmatch(r"(Add\w+)\((\w+)\);", s)
            if m:
                sub = dict(env)
                sub["surveyNode" if m.group(1) == "AddSurveyDefinitions" else "topNode"] = env[m.group(2)]
                run(helpers[m.group(1)], sub)
                continue
            m = re.fullmatch(r"config\.Save\(.*\);", s)
            if m:
                continue
            if s == "config = configNode;":
                continue
            raise SystemExit("ref: statement? " + s)

    _, stmts = cs.body("private static void CreateDefaultConfig()")
    run(stmts, env)
    contracts = wrapper.nodes[0]
    return wrapper, contracts


def parse_cfg(path):
    """KSP cfg text -> Node tree (// comments dropped)"""
    root = Node("")
    stack = [root]
    pending = None
    for raw in open(path, encoding="utf-8-sig").read().replace("\r", "").split("\n"):
        l = raw.split("//")[0].strip()
        if not l:
            continue
        if l == "{":
            n = Node(pending)
            stack[-1].add_node(n)
            stack.append(n)
            pending = None
        elif l == "}":
            stack.pop()
        elif "=" in l:
            k, v = l.split("=", 1)
            stack[-1].values.append((k.strip(), v.strip(), None))
        else:
            pending = l
    return root


def get_node(n, name):
    if n is None:
        return None
    for c in n.nodes:
        if c.name == name:
            return c
    return None


def parse_value(t, s):
    if t == "int":
        return int(s)
    if t == "float":
        return f32_from_decimal(s)
    if t == "double":
        return float(Fraction(s))
    if t == "bool":
        if s.lower() not in ("true", "false"):
            raise ValueError
        return s.lower() == "true"
    return s


def load_config(cs, defaults, contracts_node, serenity):
    """interpret LoadConfig: returns (values dict path -> value, registry list)"""
    cur = {p: v for p, (t, v) in defaults.items()}
    _, stmts = cs.body("private static void LoadConfig()")
    env = {"config": contracts_node}
    where = {"config": ("", "")}
    for st in stmts:  # static pass: which cfg node every variable names, whatever the control flow
        mm = re.fullmatch(r"(?:ConfigNode )?(\w+) = (\w+)\.GetNode\((%s)\);" % STR, st)
        if mm:
            where[mm.group(1)] = (unq(mm.group(3)), "") if mm.group(2) == "config" else (where[mm.group(2)][0], unq(mm.group(3)))
    registry = []
    skip = []
    pending_if = None
    ser_depth = []
    for s in stmts:
        if any(skip):
            if s == "{":
                skip.append(True)
                ser_depth.append(False)
            elif s == "}":
                skip.pop()
                ser_depth.pop()
            continue
        if pending_if is not None:
            if s == "{":
                skip.append(not pending_if[0])
                ser_depth.append(pending_if[1])
                pending_if = None
                continue
        m = re.fullmatch(r"if \((\w+) != null\)", s)
        if m:
            pending_if = (env.get(m.group(1)) is not None, False)
            continue
        if s == 'if (ExpansionsLoader.IsExpansionInstalled("Serenity"))':
            pending_if = (serenity, True)
            continue
        if s == "{":
            skip.append(False)
            ser_depth.append(False)
            continue
        if s == "}":
            skip.pop()
            ser_depth.pop()
            continue
        m = re.fullmatch(r"ConfigNode (\w+) = null;", s)
        if m:
            env[m.group(1)] = None
            continue
        m = re.fullmatch(r"(?:ConfigNode )?(\w+) = (\w+)\.GetNode\((%s)\);" % STR, s)
        if m:
            var, src, nm = m.group(1), m.group(2), unq(m.group(3))
            env[var] = get_node(env[src], nm)
            continue
        m = re.fullmatch(r"SystemUtilities\.LoadNode\((\w+), (%s), (%s), ref ([\w.]+), ([\w.]+)\);" % (STR, STR), s)
        if m:
            var, path = m.group(1), m.group(4)
            t = defaults[path][0]
            key = path.split(".")[-1]
            sec, sub = where[var]
            registry.append([sec, sub, key, path, any(ser_depth)])
            node = env[var]
            if node is not None:
                for k, v, _ in node.values:
                    if k == key:
                        try:
                            cur[path] = parse_value(t, v)
                        except (ValueError, ArithmeticError):
                            pass
                        break
            continue
        raise SystemExit("ref LoadConfig: statement? " + s)
    return cur, registry


def enc(t, v):
    if t == "float":
        return "%08x" % bits32(v)
    if t == "double":
        return "%016x" % bits64(v)
    if t == "bool":
        return int(bool(v))
    if t == "int":
        return int(v)
    return v if v is not None else ""


def build(cs_root, cfg_path):
    cs = CS(cs_root + "/FinePrint/ContractDefs.cs")
    defaults = parse_defaults(cs)
    wrapper, contracts = build_tree(cs, defaults)
    text = save_text(wrapper)
    cfg = parse_cfg(cfg_path)
    cfg_contracts = get_node(cfg, "Contracts")
    loaded0, registry = load_config(cs, defaults, cfg_contracts, False)
    loaded1, _ = load_config(cs, defaults, cfg_contracts, True)
    # the written tree read back by LoadConfig (the writer / reader round trip), both Serenity states
    written = parse_cfg_text(text)
    rt0, _ = load_config(cs, defaults, get_node(written, "Contracts"), False)
    rt1, _ = load_config(cs, defaults, get_node(written, "Contracts"), True)

    def enc_all(d):
        return {p: enc(defaults[p][0], v) for p, v in d.items()}

    return {
        "types": {p: t for p, (t, _) in defaults.items()},
        "defaults": enc_all({p: v for p, (t, v) in defaults.items()}),
        "loadedShipped": enc_all(loaded0),
        "loadedShippedSerenity": enc_all(loaded1),
        "writerText": text,
        "roundTrip": enc_all(rt0),
        "roundTripSerenity": enc_all(rt1),
        "registry": registry,
        "shippedNodeNames": [n.name for n in cfg_contracts.nodes],
        "writerNodeNames": [n.name for n in contracts.nodes],
    }


def parse_cfg_text(text):
    root = Node("")
    stack = [root]
    pending = None
    # the written text is the Contracts node and its children without a wrapper: wrap as a node named by its first line
    for raw in text.replace("\r", "").split("\n"):
        l = raw.split("//")[0].strip()
        if not l:
            continue
        if l == "{":
            n = Node(pending)
            stack[-1].add_node(n)
            stack.append(n)
            pending = None
        elif l == "}":
            stack.pop()
        elif "=" in l:
            k, v = l.split("=", 1)
            stack[-1].values.append((k.strip(), v.strip(), None))
        else:
            pending = l
    return root
