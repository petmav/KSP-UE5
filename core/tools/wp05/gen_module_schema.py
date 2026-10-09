"""Generate include/ksp/parts/module_schema.inc: the PartModule class table of Assembly-CSharp.

    python -I tools/wp05/gen_module_schema.py <core dir>

For every class that is PartModule or derives from it (transitively, by simple base-type name), in file order:
    KSP_MODULE_CLASS(id, "Name", "Namespace.Outer.Name", "BaseName", isAbstract, "file", line)
    KSP_MODULE_FIELD(id, "field", "csType", "TypeName", isPersistant, isProperty, isPrivate, "initializer", line)
...one KSP_MODULE_FIELD per member declared IN that class carrying [KSPField] or [KSPAxisField] (KSPAxisField derives
from KSPField; BaseFieldList.ReflectedData matches attributes by GetCustomAttributes(typeof(KSPField), inherit: true)).
Members are emitted in declaration order. `initializer` is the C# initializer expression text ("" when none).
isPrivate marks private members: reflection with FlattenHierarchy does not return private members of base classes, so a
private [KSPField] of a base class is not part of a derived class's BaseFieldList (kspcore applies that rule).

This is the reflection metadata AssemblyLoader / BaseFieldList would see (WP-05 uses the class hierarchy, the field names
and types to type the MODULE values of part configs; WP-06 consumes the same table for module instantiation).
Regenerate, never hand-edit.
"""
import os
import re
import sys

ACCESS = ("public", "private", "protected", "internal")
MODIFIERS = ("static", "readonly", "const", "new", "volatile", "override", "virtual", "abstract", "sealed", "unsafe",
             "extern", "partial", "event")

CLASS_RE = re.compile(r"^\s*(?:(?:public|private|protected|internal|static|sealed|abstract|partial|new|unsafe)\s+)*"
                      r"(class|struct|interface|enum)\s+(\w+)(?:<[^>]*>)?\s*(?::\s*([^{]+?))?\s*(?:where\s.*)?$")
NS_RE = re.compile(r"^\s*namespace\s+([\w.]+)")

TYPE_NAME = {"float": "Single", "double": "Double", "int": "Int32", "uint": "UInt32", "long": "Int64",
             "ulong": "UInt64", "short": "Int16", "ushort": "UInt16", "byte": "Byte", "sbyte": "SByte", "bool": "Boolean",
             "string": "String", "char": "Char", "object": "Object", "decimal": "Decimal"}


def strip_code(line, state):
    """Remove string/char literals and comments; `state` carries an open /* comment or verbatim string."""
    out = []
    i, n = 0, len(line)
    while i < n:
        if state.get("block"):
            j = line.find("*/", i)
            if j < 0:
                return "".join(out)
            state["block"] = False
            i = j + 2
            continue
        if state.get("verbatim"):
            j = i
            while True:
                j = line.find('"', j)
                if j < 0:
                    return "".join(out)
                if j + 1 < n and line[j + 1] == '"':
                    j += 2
                    continue
                break
            state["verbatim"] = False
            out.append('""')
            i = j + 1
            continue
        c = line[i]
        if c == "/" and i + 1 < n and line[i + 1] == "/":
            break
        if c == "/" and i + 1 < n and line[i + 1] == "*":
            state["block"] = True
            i += 2
            continue
        if c == "@" and i + 1 < n and line[i + 1] == '"':
            state["verbatim"] = True
            i += 2
            continue
        if c == "$" and i + 1 < n and line[i + 1] == '"':
            i += 1
            continue
        if c == '"':
            j = i + 1
            while j < n and line[j] != '"':
                j += 2 if line[j] == "\\" else 1
            out.append('""')
            i = j + 1
            continue
        if c == "'":
            j = i + 1
            while j < n and line[j] != "'":
                j += 2 if line[j] == "\\" else 1
            out.append("' '")
            i = j + 1
            continue
        out.append(c)
        i += 1
    return "".join(out)


def cs_type_name(t):
    t = t.strip()
    if t.endswith("?"):
        return "Nullable`1"
    if t.endswith("[]"):
        return cs_type_name(t[:-2]) + "[]"
    if "<" in t:
        base = t[:t.index("<")].split(".")[-1]
        args = t[t.index("<") + 1:t.rindex(">")]
        depth, count = 0, 1
        for ch in args:
            if ch == "<":
                depth += 1
            elif ch == ">":
                depth -= 1
            elif ch == "," and depth == 0:
                count += 1
        return "%s`%d" % (base, count)
    return TYPE_NAME.get(t, t.split(".")[-1])


def parse_member(decl):
    """decl: text of a member declaration up to ';' or '{' or '=>'. -> (access, mods, type, name, init, isProperty)"""
    s = decl.strip()
    is_prop = False
    init = ""
    if "=>" in s and not re.search(r"=\s*[^>]", s.split("=>")[0]):
        s = s.split("=>")[0].strip()
        is_prop = True
    elif s.endswith("{") or s.endswith("{ get; set; }") or "{ get" in s:
        s = s.split("{")[0].strip()
        is_prop = True
    else:
        s = s.rstrip(";").strip()
        m = re.match(r"^(.*?\w(?:<[^=]*>)?(?:\[\])?\s+\w+)\s*=\s*(.*)$", s)
        if m:
            s, init = m.group(1), m.group(2).strip()
    toks = s.split()
    access, mods = "private", []
    while toks and (toks[0] in ACCESS or toks[0] in MODIFIERS):
        t = toks.pop(0)
        if t in ACCESS:
            access = t if access == "private" or t == "public" else access
        else:
            mods.append(t)
    if len(toks) < 2:
        return None
    name = toks[-1]
    typ = " ".join(toks[:-1])
    if not re.match(r"^\w+$", name) or "(" in typ or "(" in name:
        return None
    return access, mods, typ, name, init, is_prop


def scan(root):
    classes = []  # dict(name, full, base, abstract, file, line, members)
    for dp, dn, fn in os.walk(root):
        dn.sort()
        for f in sorted(fn):
            if not f.endswith(".cs"):
                continue
            path = os.path.join(dp, f)
            rel = os.path.relpath(path, root).replace("\\", "/")
            lines = open(path, encoding="utf-8-sig", errors="replace").read().splitlines()
            state = {}
            depth = 0
            stack = []  # (class dict or None, depth at body)
            ns_stack = []
            pending_attrs = []
            pending_decl = None
            for ln, raw in enumerate(lines, 1):
                code = strip_code(raw, state)
                s = code.strip()
                body_owner = stack[-1][0] if stack and stack[-1][2] and stack[-1][1] == depth else None
                if body_owner is not None and body_owner["kind"] in ("enum", "interface"):
                    body_owner = None
                if pending_decl is not None:
                    pending_decl[0] += " " + s
                    if ";" in s or "{" in s or "=>" in s:
                        text, start = pending_decl
                        pending_decl = None
                        if body_owner is not None:
                            r = parse_member(text)
                            if r:
                                body_owner["members"].append((r, list(pending_attrs), start))
                        pending_attrs = []
                elif s:
                    m = NS_RE.match(s)
                    cm = CLASS_RE.match(s) if not s.startswith("[") else None
                    if m:
                        ns_stack.append((m.group(1), depth))
                    elif cm:
                        kind, name, bases = cm.group(1), cm.group(2), cm.group(3)
                        base = None
                        if kind == "class" and bases:
                            base = bases.split(",")[0].strip()
                            base = re.sub(r"<.*>", "", base).split(".")[-1]
                        outer = [c[0]["name"] for c in stack if c[0] is not None and c[2]]
                        ns = ".".join(n for n, _ in ns_stack)
                        full = ".".join([x for x in [ns] + outer + [name] if x])
                        cls = {"name": name, "full": full, "base": base, "kind": kind,
                               "abstract": " abstract " in " " + s + " ", "file": rel, "line": ln, "members": [], "methods": []}
                        if kind in ("class", "enum"):
                            classes.append(cls)
                        stack.append([cls, depth + 1, False])
                        pending_attrs = []
                    elif s.startswith("["):
                        if body_owner is not None:
                            pending_attrs.append(s)
                    elif body_owner is not None and re.match(r"^[A-Za-z_]", s) and "(" not in s.split("=")[0]:
                        if body_owner is not None:
                            if ";" in s or "{" in s or "=>" in s:
                                r = parse_member(s)
                                if r:
                                    body_owner["members"].append((r, list(pending_attrs), ln))
                            else:
                                pending_decl = [s, ln]
                                continue
                        pending_attrs = []
                    elif body_owner is not None and pending_attrs and re.match(r"^[A-Za-z_]", s) and "(" in s:
                        # WP-06: attributed methods ([KSPEvent] / [KSPAction]) for tools/wp06/gen_module_members.py
                        body_owner["methods"].append((s, list(pending_attrs), ln))
                        pending_attrs = []
                    else:
                        pending_attrs = []
                depth += code.count("{") - code.count("}")
                for e in stack:
                    if not e[2] and depth >= e[1]:
                        e[2] = True
                while stack and stack[-1][2] and depth < stack[-1][1]:
                    stack.pop()
                while ns_stack and depth <= ns_stack[-1][1] and "}" in code:
                    ns_stack.pop()
    return classes


def is_ksp_field_attr(a):
    return re.match(r"^\[(KSPField|KSPAxisField)\b", a) is not None


def persistant(attrs):
    for a in attrs:
        if is_ksp_field_attr(a) and re.search(r"isPersistant\s*=\s*true", a):
            return True
    return False


def main(core):
    root = None
    p = os.path.abspath(core)
    for _ in range(10):
        cand = os.path.join(p, "decompiled", "KSP-Decompiled", "Assembly-CSharp")
        if os.path.isdir(cand):
            root = cand
            break
        p = os.path.dirname(p)
    if root is None:
        raise SystemExit("decompiled/KSP-Decompiled/Assembly-CSharp not found")
    # strip_code removes string contents, so initializers are re-read from the raw source line below
    allc = scan(root)
    classes = [c for c in allc if c["kind"] == "class"]
    enums = [c for c in allc if c["kind"] == "enum"]
    unity_root = os.path.join(os.path.dirname(root), "Unity")
    by_name = {}
    for c in classes:
        by_name.setdefault(c["name"], []).append(c)

    def derives_from(c, rootName, seen=None):
        seen = seen or set()
        if c["name"] == rootName and c["full"] == rootName:
            return True
        b = c["base"]
        if not b or b in seen:
            return False
        seen.add(b)
        for bc in by_name.get(b, []):
            if derives_from(bc, rootName, seen):
                return True
        return False

    mods = [c for c in classes if derives_from(c, "PartModule")]
    raw_cache = {}

    def raw_line(rel, ln):
        if rel not in raw_cache:
            raw_cache[rel] = open(os.path.join(root, rel), encoding="utf-8-sig", errors="replace").read().splitlines()
        return raw_cache[rel][ln - 1]

    def esc(s):
        return s.replace("\\", "\\\\").replace('"', '\\"')

    out = ["// Generated by tools/wp05/gen_module_schema.py from decompiled/KSP-Decompiled/Assembly-CSharp. Do not edit.",
           "// KSP_MODULE_ENUM_BEGIN(eid, name, fullName, isFlags, file, line) KSP_MODULE_ENUM_MEMBER(eid, name, value)",
           "// KSP_MODULE_ENUM_END(eid)",
           "// KSP_MODULE_CLASS(id, name, fullName, baseName, isAbstract, file, line)",
           "// KSP_MODULE_FIELD(id, field, csType, typeName, isPersistant, isProperty, isPrivate, initializer, line, enumId)",
           "// KSP_MODULE_CLASS_END(id)"]

    def enum_members(path, line):
        lines = open(path, encoding="utf-8-sig", errors="replace").read().splitlines()
        flags = line >= 2 and "[Flags]" in lines[line - 2]
        j = line
        while "{" not in lines[j - 1]:
            j += 1
        members, val = [], -1
        for l2 in lines[j:]:
            t = l2.split("//")[0].strip()
            if t.startswith("}"):
                break
            if not t or t.startswith("["):
                continue
            t = t.rstrip(",")
            if "=" in t:
                n, v = [x.strip() for x in t.split("=", 1)]
                try:
                    val = int(v.rstrip("uL"), 0)
                except ValueError:
                    env = {m: x for m, x in members}
                    val = eval(v.replace("|", " | "), {}, env)
            else:
                n = t
                val += 1
            members.append((n, val))
        return flags, members

    def resolve_enum(typ, owner):
        t = typ.split(".")[-1]
        cands = [e for e in enums if e["name"] == t]
        chain = []
        c = owner
        while c is not None:
            chain.append(c["full"])
            nb = by_name.get(c["base"] or "", [])
            c = nb[-1] if nb else None
        for full in chain:
            for e in cands:
                if e["full"] == full + "." + t:
                    return e, os.path.join(root, e["file"])
        tops = [e for e in cands if e["full"] == t or e["full"].endswith("." + t) and e["full"].count(".") == 1 and False]
        tops = [e for e in cands if "." not in e["full"]] or cands
        if len(tops) >= 1 and (len(tops) == 1 or len(set(x["full"] for x in tops)) == 1):
            return tops[0], os.path.join(root, tops[0]["file"])
        upath = None
        for dp, dn, fn in os.walk(unity_root):
            if t + ".cs" in fn:
                upath = os.path.join(dp, t + ".cs")
                break
        if upath:
            src = open(upath, encoding="utf-8-sig", errors="replace").read().splitlines()
            for i, l in enumerate(src, 1):
                if re.match(r"^\s*public enum %s\b" % t, l):
                    return {"name": t, "full": "UnityEngine." + t, "file": os.path.relpath(upath, os.path.dirname(root)).replace("\\", "/"), "line": i, "unity": True}, upath
        return None, None
    nf = 0
    ids = {}
    enum_ids = {}
    enum_out = []

    def enum_id_for(typ, owner):
        if typ.split(".")[-1] in ("string", "bool", "float", "double", "int", "uint", "long", "ulong", "short", "ushort",
                                  "byte", "Vector2", "Vector3", "Vector4", "Quaternion", "FloatCurve", "DoubleCurve", "Color"):
            return "nullptr"
        e, path = resolve_enum(typ, owner)
        if e is None:
            return "nullptr"
        eid = "kModuleEnum_" + re.sub(r"\W", "_", e["full"])
        if eid not in enum_ids:
            flags, members = enum_members(path, e["line"])
            enum_ids[eid] = 1
            enum_out.append('KSP_MODULE_ENUM_BEGIN(%s, "%s", "%s", %s, "%s", %d)' % (
                eid, e["name"], e["full"], "true" if flags else "false", e["file"], e["line"]))
            for n, v in members:
                enum_out.append('KSP_MODULE_ENUM_MEMBER(%s, "%s", %d)' % (eid, n, v))
            enum_out.append("KSP_MODULE_ENUM_END(%s)" % eid)
        return "&" + eid

    def emit_classes(mods, out, cls_macro="KSP_MODULE_CLASS", fld_macro="KSP_MODULE_FIELD", end_macro="KSP_MODULE_CLASS_END"):
        nonlocal nf
        for c in mods:
            cid = re.sub(r"\W", "_", c["full"])
            if cid in ids:
                cid += "_%d" % c["line"]
            ids[cid] = 1
            out.append(cls_macro + '(%s, "%s", "%s", "%s", %s, "%s", %d)' % (
                cid, c["name"], c["full"], c["base"] or "", "true" if c["abstract"] else "false", c["file"], c["line"]))
            for (access, modsx, typ, name, init, is_prop), attrs, ln in c["members"]:
                if "static" in modsx or "const" in modsx:
                    continue
                if not any(is_ksp_field_attr(a) for a in attrs):
                    continue
                if init:
                    rl = raw_line(c["file"], ln)
                    mm = re.search(r"\b%s\s*=\s*(.*?);\s*(//.*)?$" % re.escape(name), rl)
                    if mm:
                        init = mm.group(1).strip()
                out.append(fld_macro + '(%s, "%s", "%s", "%s", %s, %s, %s, "%s", %d, %s)' % (
                    cid, name, esc(typ), cs_type_name(typ), "true" if persistant(attrs) else "false",
                    "true" if is_prop else "false", "true" if access == "private" else "false", esc(init), ln,
                    enum_id_for(typ, c)))
                nf += 1
            out.append(end_macro + "(%s)" % cid)
    emit_classes(mods, out)
    out = out[:6] + enum_out + out[6:]
    inc = os.path.join(core, "include", "ksp", "parts")
    with open(os.path.join(inc, "module_schema.inc"), "w", newline="\n", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print("module classes:", len(mods), "KSPField members:", nf)

    # WP-06: the same schema for the ScenarioModule and VesselModule class trees (host_schema.inc). Enum tables already
    # emitted for module_schema.inc are referenced, not redefined (both files are included in one translation unit).
    hosts = [c for c in classes if derives_from(c, "ScenarioModule") or derives_from(c, "VesselModule")]
    hout = ["// Generated by tools/wp05/gen_module_schema.py from decompiled/KSP-Decompiled/Assembly-CSharp. Do not edit.",
            "// ScenarioModule and VesselModule class trees (WP-06). Same macros as module_schema.inc but KSP_HOST_*;",
            "// host root classes (ScenarioModule, VesselModule) are included (the roots have no [KSPField])."]
    enum_out.clear()
    nf = 0
    emit_classes(hosts, hout, "KSP_HOST_CLASS", "KSP_HOST_FIELD", "KSP_HOST_CLASS_END")
    hout = hout[:3] + enum_out + hout[3:]
    with open(os.path.join(inc, "host_schema.inc"), "w", newline="\n", encoding="utf-8") as f:
        f.write("\n".join(hout) + "\n")
    print("host classes:", len(hosts), "KSPField members:", nf)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")
