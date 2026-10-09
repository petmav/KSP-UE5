#!/usr/bin/env python3
"""Auto-instrument Unity callback methods in the decompiled Assembly-CSharp for the
`profiling` branch of KSP-Decompiled.

For every method whose signature line is exactly
    <indent>[modifiers] void (FixedUpdate|Update|LateUpdate|OnCollisionEnter|OnCollisionStay|OnCollisionExit|OnJointBreak)(...)
followed by `<indent>{` and closed by the first later line equal to `<indent>}`, the body is
wrapped as

    {
        long __pp0 = global::PhysProf.Begin();
        try
        {
            <original body, not re-indented>
        }
        finally
        {
            global::PhysProf.End(<id>, __pp0);
        }
    }

Ids start at PhysProf.FirstAuto (100). A name table is written to PhysProf/PhysProfNames.cs.
Third-party embedded code (TMPro, VehiclePhysics, EdyCommonTools, Smooth, SoftMasking, UniLinq,
LibNoise, Highlighting, UnityEngine, A) is skipped. Line endings and BOMs are preserved.

Usage: python physprof_instrument.py <Assembly-CSharp dir> [--dry-run]
"""
import os
import re
import sys

FIRST_AUTO = 100
SKIP_DIRS = {"TMPro", "VehiclePhysics", "EdyCommonTools", "Smooth", "SoftMasking", "UniLinq",
             "LibNoise", "Highlighting", "UnityEngine", "A", "PhysProf", "obj", "bin", "Properties"}
SIG = re.compile(r"^(\t+)((?:public |protected |private |internal )*(?:virtual |override |new |sealed )*)void "
                 r"(FixedUpdate|Update|LateUpdate|OnCollisionEnter|OnCollisionStay|OnCollisionExit|OnJointBreak)"
                 r"\((Collision \w+|float \w+)?\)\s*$")
CLASS = re.compile(r"^(\t*)(?:\[[^\]]*\]\s*)*(?:(?:public|private|protected|internal|static|sealed|abstract|partial|new|unsafe)\s+)*(?:class|struct)\s+(\w+)")
NS = re.compile(r"^namespace\s+([\w.]+)")


def enclosing_class(lines, i, indent_len):
    """Nearest preceding class/struct declaration at a smaller indentation, plus nesting chain."""
    chain = []
    want = indent_len - 1
    j = i - 1
    while j >= 0 and want >= 0:
        m = CLASS.match(lines[j])
        if m and len(m.group(1)) == want:
            chain.insert(0, m.group(2))
            want -= 1
        j -= 1
    ns = None
    for line in lines[:i]:
        m = NS.match(line)
        if m:
            ns = m.group(1)
    return ns, ".".join(chain) if chain else "?"


def main():
    root = sys.argv[1]
    dry = "--dry-run" in sys.argv
    names = []
    files_changed = 0
    for dirpath, dirnames, filenames in os.walk(root):
        rel = os.path.relpath(dirpath, root)
        top = rel.split(os.sep)[0]
        if top in SKIP_DIRS:
            dirnames[:] = []
            continue
        dirnames.sort()
        for fn in sorted(filenames):
            if not fn.endswith(".cs"):
                continue
            path = os.path.join(dirpath, fn)
            raw = open(path, "rb").read()
            text = raw.decode("utf-8")
            nl = "\r\n" if "\r\n" in text else "\n"
            lines = text.split(nl)
            out = []
            i = 0
            changed = False
            while i < len(lines):
                line = lines[i]
                m = SIG.match(line)
                if m and i + 1 < len(lines) and lines[i + 1] == m.group(1) + "{":
                    ind = m.group(1)
                    close = None
                    for k in range(i + 2, len(lines)):
                        if lines[k] == ind + "}":
                            close = k
                            break
                        if lines[k].strip() and not lines[k].startswith(ind) :
                            break
                    if close is None:
                        print("WARN: no close brace for %s:%d" % (path, i + 1), file=sys.stderr)
                        out.append(line)
                        i += 1
                        continue
                    ns, cls = enclosing_class(lines, i, len(ind))
                    mid = FIRST_AUTO + len(names)
                    qual = cls + "." + m.group(3)
                    names.append((qual, ns or "", os.path.relpath(path, root).replace(os.sep, "/"), i + 1))
                    out.append(line)
                    out.append(ind + "{")
                    out.append(ind + "\tlong __pp0 = global::PhysProf.Begin();")
                    out.append(ind + "\ttry")
                    out.append(ind + "\t{")
                    out.extend(lines[i + 2:close])
                    out.append(ind + "\t}")
                    out.append(ind + "\tfinally")
                    out.append(ind + "\t{")
                    out.append(ind + "\t\tglobal::PhysProf.End(%d, __pp0);" % mid)
                    out.append(ind + "\t}")
                    out.append(ind + "}")
                    i = close + 1
                    changed = True
                    continue
                out.append(line)
                i += 1
            if changed:
                files_changed += 1
                if not dry:
                    open(path, "wb").write(nl.join(out).encode("utf-8"))
    # disambiguate duplicate qualified names with namespace
    counts = {}
    for q, ns, p, ln in names:
        counts[q] = counts.get(q, 0) + 1
    final = []
    for q, ns, p, ln in names:
        final.append((ns + "." + q) if counts[q] > 1 and ns else q)
    gen = os.path.join(root, "PhysProf", "PhysProfNames.cs")
    body = ["// <auto-generated> by decompiled/tools/scripts/physprof_instrument.py. Profiling branch only. </auto-generated>",
            "internal static class PhysProfNames", "{",
            "\tpublic const int FirstAuto = %d;" % FIRST_AUTO,
            "\tpublic const int Count = %d;" % len(final),
            "\tpublic static readonly string[] Names = new string[]", "\t{"]
    for idx, (n, (q, ns, p, ln)) in enumerate(zip(final, names)):
        body.append("\t\t\"%s\", // %d %s:%d" % (n, FIRST_AUTO + idx, p, ln))
    body += ["\t};", "}", ""]
    if not dry:
        os.makedirs(os.path.dirname(gen), exist_ok=True)
        open(gen, "wb").write("\r\n".join(body).encode("utf-8"))
    print("instrumented methods: %d in %d files" % (len(names), files_changed))


if __name__ == "__main__":
    main()
