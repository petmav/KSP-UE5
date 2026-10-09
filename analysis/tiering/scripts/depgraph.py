"""Type-reference dependency graph between files (by simple top-level type name).

Run: python -I depgraph.py      (after extract_features.py)
Writes work/depgraph.json: {path: {"in": [paths referencing it], "out": [paths it references]}}
A file F references G if any top-level type declared in G (and declared nowhere else) appears as an
identifier in F. Ambiguous names (declared in >1 file) and names that shadow common BCL/Unity names
are skipped.
"""
import json, os
from collections import defaultdict

WORK = r"C:/Projects/KSP-UE5/analysis/tiering/work"
SKIP = {"Action", "Func", "Object", "Type", "Random", "Debug", "Time", "Input", "Event", "Mode", "State",
        "Data", "Callback", "Settings", "Node", "Item", "Info", "Result", "Status", "Value", "Values",
        "Type", "Flags", "Entry", "Path", "Link", "Version", "Text", "Image", "Button", "Toggle", "Slider",
        "Light", "Camera", "Animation", "Material", "Mesh", "Shader", "Texture", "Color", "Rect", "Bounds",
        "Vector2", "Vector3", "Vector4", "Quaternion", "Matrix4x4", "Mathf", "List", "Dictionary", "Tuple",
        "Option", "Either", "Unit", "Enumerable", "Comparer", "EqualityComparer", "Collections", "Utils",
        "Util", "Utilities", "Extensions", "Constants", "Tools", "GUI", "Screen", "Cursor", "AA", "A"}


def main():
    rows = [json.loads(l) for l in open(os.path.join(WORK, "features.jsonl"), encoding="utf-8")]
    decl_files = defaultdict(set)
    for r in rows:
        ns_depth = 1 if r["namespace"] else 0
        for d in r["decls"]:
            if d["depth"] == ns_depth:
                decl_files[d["name"]].add(r["path"])
    owner = {n: next(iter(fs)) for n, fs in decl_files.items() if len(fs) == 1 and n not in SKIP and len(n) > 2}
    ns_of = {r["path"]: r["namespace"] for r in rows}

    def visible(owner_path, r):
        ns = ns_of[owner_path]
        if not ns:
            return True
        mine = r["namespace"]
        if mine == ns or mine.startswith(ns + "."):  # same or enclosing namespace
            return True
        return ns in r["usings"]
    out_edges = {}
    in_edges = defaultdict(set)
    for r in rows:
        refs = set()
        for ident in r["idents"]:
            f = owner.get(ident)
            if f and f != r["path"] and visible(f, r):
                refs.add(f)
        out_edges[r["path"]] = sorted(refs)
        for f in refs:
            in_edges[f].add(r["path"])
    res = {r["path"]: {"in": sorted(in_edges.get(r["path"], ())), "out": out_edges[r["path"]]} for r in rows}
    with open(os.path.join(WORK, "depgraph.json"), "w", encoding="utf-8") as fh:
        json.dump(res, fh)
    top = sorted(res.items(), key=lambda kv: -len(kv[1]["in"]))[:60]
    for p, e in top:
        print(len(e["in"]), p)
    amb = sorted(((n, len(fs)) for n, fs in decl_files.items() if len(fs) > 1), key=lambda x: -x[1])
    print("ambiguous top-level names:", len(amb), amb[:40])


if __name__ == "__main__":
    main()
