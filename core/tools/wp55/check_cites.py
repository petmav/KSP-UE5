#!/usr/bin/env python3
"""WP-55 wrapper around tools/wp28/check_cites.py (exact mode) with the two rules the node library needs, and a cite resolver.

* A C++ constructor `Class::Class(` ports the C# `Awake()` (MonoBehaviour classes of Tests/ and Actions/) or the C# constructor; the cite may name
  either, so the candidates of a constructor are the declaration lines of `Class`, of `Awake` and of the class itself.
* A C++ destructor `Class::~Class(` ports `OnDestroy`.
* `--resolve`: rewrites placeholder cites `// File.cs:@Name` / `// File.cs:@Name#k` (k-th declaration, default 1) to the line number, so the
  .cpp can be written without counting lines.

Usage: python -I tools/wp55/check_cites.py [--resolve] [--cs-root DIR] file.cpp ...
"""
import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("cc28", os.path.join(HERE, "..", "wp28", "check_cites.py"))
cc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cc)

_orig = cc.candidates
CTOR = re.compile(r"\b(?:public|internal|protected|private)\s+(\w+)\s*\(")


def candidates(lines, name):
    out = list(_orig(lines, name))
    # constructor ports: Awake / OnDestroy lines and the constructors of this file
    for alias in ("Awake", "OnDestroy"):
        pass
    return out


def wrap_process(path, cs_root, fix, window=None):
    return cc.process(path, cs_root, fix, window)


def patched_process(path, cs_root, fix, window=None):
    src = open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    lines = src.split(nl)
    # rewrite the NAME of constructors / destructors to what the base checker can find: temporary text with Awake / OnDestroy names
    tmp = []
    for l in lines:
        m = re.search(r"\b(\w+)::(\1)\s*\(", l)
        if m and not l.lstrip().startswith("//"):
            l = l.replace("%s::%s(" % (m.group(1), m.group(2)), "%s::__CTOR__(" % m.group(1), 1)
        m = re.search(r"\b(\w+)::~(\1)\s*\(", l)
        if m and not l.lstrip().startswith("//"):
            l = l.replace("%s::~%s(" % (m.group(1), m.group(2)), "%s::OnDestroy(" % m.group(1), 1)
        tmp.append(l)
    cc.NAME_ALIAS["__CTOR__"] = "__CTOR__"
    # candidates for __CTOR__: Awake or any constructor of the file (the C# constructor named like the class)
    base = cc.candidates

    def cand(clines, name):
        if name == "__CTOR__":
            out = [i + 1 for i, l in enumerate(clines) if re.search(r"\bvoid\s+Awake\s*\(", l) or CTOR.search(l)]
            return sorted(set(out))
        return base(clines, name)

    cc.candidates = cand
    tp = path + ".__tmp__.cpp"
    open(tp, "w", encoding="utf-8", newline="").write(nl.join(tmp))
    try:
        bad, total = cc.process(tp, cs_root, False, window)
    finally:
        cc.candidates = base
        os.remove(tp)
    return bad, total


def resolve(path, cs_root):
    src = open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    lines = src.split(nl)
    pat = re.compile(r"^(\s*// )([A-Za-z0-9_/]+)\.cs:@(~?\w+)(?:#(\d+))?")
    miss = 0
    for i, l in enumerate(lines):
        m = pat.match(l)
        if not m:
            continue
        f, name, k = m.group(2), m.group(3), int(m.group(4) or 1)
        cs = cc.load(cs_root, f)
        if cs is None:
            print("%s:%d: no C# file %s" % (path, i + 1, f))
            miss += 1
            continue
        if name == "Class":
            c = [j + 1 for j, t in enumerate(cs) if re.search(r"\b(class|enum|interface|struct)\s+%s\b" % re.escape(f.split("/")[-1]), t)]
        elif name in ("Awake", "ctor"):
            c = [j + 1 for j, t in enumerate(cs) if re.search(r"\bvoid\s+Awake\s*\(", t)] if name == "Awake" else \
                [j + 1 for j, t in enumerate(cs) if CTOR.search(t)]
        else:
            c = cc.candidates(cs, name)
        if len(c) < k:
            print("%s:%d: %s.cs has no %s#%d (%s)" % (path, i + 1, f, name, k, c))
            miss += 1
            continue
        lines[i] = pat.sub(lambda mm: "%s%s.cs:%d" % (mm.group(1), f, c[k - 1]), l, count=1)
    open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
    return miss


def main():
    args = sys.argv[1:]
    cs = cc.DEFAULT_CS
    if "--cs-root" in args:
        k = args.index("--cs-root")
        cs = args[k + 1]
        del args[k:k + 2]
    res = "--resolve" in args
    files = [a for a in args if not a.startswith("--")]
    bad = total = 0
    for f in files:
        if res:
            bad += resolve(f, cs)
            continue
        b, t = patched_process(f, cs, False)
        bad += b
        total += t
    print("%d cites checked, %d mismatches" % (total, bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
