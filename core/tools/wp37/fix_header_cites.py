#!/usr/bin/env python3
"""WP-37: snap the trailing `// :N` / `// File.cs:N` citations of the contracts headers to the declaration line of the member in the decompiled C#.

check_cites.py (tools/wp28) checks .cpp function definitions only; the headers cite the declaration line of each member after it (class
comment with an explicit file name sets the file, later `// :N` comments use it). The member name is the identifier before `(` / `=` / `;` on
the line. Nearest declaration line of that name in the C# file wins; no declaration found: the number is dropped (`// (C++ addition)`).

Usage: python -I core/tools/wp37/fix_header_cites.py [--check] header.hpp ...
"""
import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("check_cites", os.path.join(HERE, "..", "wp28", "check_cites.py"))
cc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cc)

CITE = re.compile(r"//\s*(?:((?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+)\.cs)?:(\d+)\b")
CLASS = re.compile(r"\b(?:class|struct)\s+(\w+)")
KEYWORDS = {"override", "static_cast", "if", "return", "const", "static", "inline", "virtual", "template", "sizeof"}


def member_name(code):
    code = code.strip()
    m = CLASS.search(code)
    if m and ("{" in code or code.endswith(":")):
        return ("class", m.group(1))
    for m in re.finditer(r"(~?\w+)\s*\(", code):
        if m.group(1) not in KEYWORDS:
            return ("fn", m.group(1))
    m = re.search(r"(\w+)\s*(?:=|;|\{)", code)
    if m:
        return ("field", m.group(1))
    return None


def decl_lines(lines, kind, name):
    out = []
    for i, l in enumerate(lines):
        s = l.strip()
        if s.startswith(("//", "using ")):
            continue
        if kind == "class":
            if re.search(r"\b(class|struct|enum)\s+" + re.escape(name) + r"\b", l):
                out.append(i + 1)
        elif re.search(r"\b" + re.escape(name) + r"\b", l) and (
                re.search(r"\b" + re.escape(name) + r"\s*(\(|=>|\{|=|;|$)", l) and
                (re.match(r"\s*(public|protected|private|internal|static|override|virtual|new|\[|[A-Za-z<>\[\],? ]+\s+" + re.escape(name) + r")", l))):
            out.append(i + 1)
    return out


def process(path, check_only):
    cs_root = cc.DEFAULT_CS
    lines = open(path, encoding="utf-8", newline="").read().split("\n")
    cur = None
    changed = bad = total = 0
    for i, l in enumerate(lines):
        m = CITE.search(l)
        if not m:
            continue
        code = l[:m.start()]
        fname, n = m.group(1), int(m.group(2))
        if fname:
            cur = fname
        if cur is None:
            continue
        mn = member_name(code)
        if mn is None:
            continue
        kind, name = mn
        cs = cc.load(cs_root, cur)
        if cs is None:
            continue
        total += 1
        cands = decl_lines(cs, kind, name)
        if not cands:
            new = None
        else:
            new = min(cands, key=lambda c: abs(c - n))
        if new == n:
            continue
        bad += 1
        if check_only:
            print("%s:%d: %s %s cite :%d -> %s" % (path, i + 1, kind, name, n, new))
            continue
        if new is None:
            lines[i] = l[:m.start()] + "// (C++ addition)" + l[m.end():]
        else:
            lines[i] = l[:m.start()] + "// " + (m.group(1) + ".cs" if fname else "") + ":" + str(new) + l[m.end():]
        changed += 1
    if not check_only and changed:
        open(path, "w", encoding="utf-8", newline="").write("\n".join(lines))
    print("%s: %d cites checked, %d %s" % (path, total, bad, "mismatches" if check_only else "fixed"))
    return bad


if __name__ == "__main__":
    args = sys.argv[1:]
    check = "--check" in args
    files = [a for a in args if not a.startswith("--")]
    sys.exit(1 if sum(process(f, check) for f in files) and check else 0)
