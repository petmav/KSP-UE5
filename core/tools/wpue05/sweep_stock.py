#!/usr/bin/env python
"""WP-UE-05 local sweep over the game's own files: every .mu and every texture under <game>/GameData.

Run:  python -I core/tools/wpue05/sweep_stock.py --game <KSP root> --out core/data/wpue05_stock_sweep.json [--tool <mu_sweep exe>]

What it does (both paths see the SAME files; this script prints only counts and digests, never file contents):
  1. runs the C++ tool mu_sweep (tests/capi/mu_sweep.cpp: kspcore through the C ABI) over <game>/GameData;
  2. parses every .mu with the independent python reference (tools/wpue05/mu_render_reference.py: own parser from PartReader.cs) and
     decodes every texture header with its reference (ref_header);
  3. cross-checks: parse status, the compact row digest (every value of every chunk: the digest covers the vertex / uv / triangle ...
     arrays as FNV-1a 64 of the raw bytes), the chunk counts, and every texture header field;
  4. writes the derived data: per file the origin path (relative to <game>) and sha256, the digest and the chunk counts; per texture the
     decoded header; a summary (shader names, versions, ignored entry types, texture formats).
Exit status 1 when the two implementations disagree or a stock file fails to parse.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mu_render_reference as ref  # noqa: E402

TEX_EXT = (".dds", ".mbm", ".png", ".jpg", ".jpeg", ".tga", ".truecolor")
FIELDS = [k for k in ref.blank_header().keys() if k != "error"]
COUNT_KEYS = ["nodes", "colliders", "mesh_filters", "mesh_renderers", "skinned_renderers", "lights", "cameras", "particle_emitters", "animations",
              "clips", "curves", "keys", "tag_and_layer", "vertices", "triangles", "submeshes", "bind_poses", "materials", "textures"]


def find_tool(explicit):
    if explicit:
        return explicit
    core = os.path.normpath(os.path.join(HERE, "..", ".."))
    for sub in ("build/RelWithDebInfo/tests/mu_sweep.exe", "build/tests/mu_sweep", "build/tests/Release/mu_sweep.exe", "build/tests/mu_sweep.exe"):
        p = os.path.join(core, sub)
        if os.path.exists(p):
            return p
    raise SystemExit("mu_sweep not found; build it (core/run_tests.ps1 -BuildOnly) or pass --tool")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--tool")
    args = ap.parse_args()
    game = os.path.abspath(args.game)
    gd = os.path.join(game, "GameData")
    tool = find_tool(args.tool)
    tmp = os.path.join(tempfile.mkdtemp(prefix="wpue05_"), "cpp.json")
    subprocess.run([tool, "--game", game, "--out", tmp], check=True)
    with open(tmp) as f:
        cpp = json.load(f)
    cpp_models = {m["path"]: m for m in cpp["models"]}
    cpp_tex = {t[0]: t for t in cpp["textures"]}

    mus, texs = [], []
    for root, _, files in os.walk(gd):
        for fn in files:
            low = fn.lower()
            p = os.path.join(root, fn)
            rel = os.path.relpath(p, game).replace("\\", "/")
            if low.endswith(".mu"):
                mus.append((rel, p))
            elif low.endswith(TEX_EXT):
                texs.append((rel, p))
    mus.sort()
    texs.sort()
    problems = []
    models = []
    shaders, versions, ignored, ignored_mesh, errors = {}, {}, {}, {}, []
    totals = {k: 0 for k in COUNT_KEYS}
    warnings = []
    for rel, p in mus:
        with open(p, "rb") as f:
            data = f.read()
        sha = hashlib.sha256(data).hexdigest()
        c = cpp_models.get(rel)
        try:
            m = ref.parse_mu(data)
            py_ok = True
        except (ref.ParseError, RecursionError) as e:
            py_ok = False
            py_err = str(e)
        if c is None:
            problems.append("%s: missing from the C++ sweep" % rel)
            continue
        if not py_ok or not c["ok"]:
            errors.append({"path": rel, "sha256": sha, "python": None if py_ok else py_err, "cpp": None if c["ok"] else c["error"]})
            if py_ok != c["ok"]:
                problems.append("%s: parse status differs (python ok=%s, C++ ok=%s)" % (rel, py_ok, c["ok"]))
            continue
        parent = os.path.relpath(os.path.dirname(p), gd).replace("\\", "/")
        rows = ref.rows(m, parent, False)
        dg = "%016x" % ref.digest(rows)
        st = ref.stats(m)
        if dg != c["digest"]:
            problems.append("%s: digest differs (python %s, C++ %s)" % (rel, dg, c["digest"]))
        for k in COUNT_KEYS:
            if st[k] != c["counts"][k]:
                problems.append("%s: count %s differs (python %d, C++ %d)" % (rel, k, st[k], c["counts"][k]))
        sh = {}
        for mat in m.materials:
            sh[mat["shader"]] = sh.get(mat["shader"], 0) + 1
        if sh != c["shaders"]:
            problems.append("%s: shader histogram differs" % rel)
        for k, v in sh.items():
            shaders[k] = shaders.get(k, 0) + v
        versions[m.version] = versions.get(m.version, 0) + 1
        for k, v in m.ignored.items():
            ignored[k] = ignored.get(k, 0) + v
        for k, v in m.ignored_mesh.items():
            ignored_mesh[k] = ignored_mesh.get(k, 0) + v
        for w in m.warnings:
            warnings.append("%s: %s" % (rel, w))
        for k in COUNT_KEYS:
            totals[k] += st[k]
        models.append({"path": rel, "sha256": sha, "size": len(data), "version": m.version, "digest": dg,
                       "counts": {k: st[k] for k in COUNT_KEYS if st[k]}, "shaders": sh})
    # textures
    tex_rows = []
    by_container, by_format = {}, {}
    payload = {"match": 0, "mismatch": 0}
    unloadable = []
    names = {1: "dds", 2: "mbm", 3: "png", 4: "jpeg", 5: "tga", 6: "truecolor"}
    for rel, p in texs:
        with open(p, "rb") as f:
            data = f.read()
        sha = hashlib.sha256(data).hexdigest()
        want = ref.ref_header(rel, data, len(data))
        c = cpp_tex.get(rel)
        if c is None:
            problems.append("%s: texture missing from the C++ sweep" % rel)
            continue
        got = c[2]
        got_err = got[len(FIELDS)]
        for i, k in enumerate(FIELDS):
            if got[i] != want[k]:
                problems.append("%s: header field %s differs (python %s, C++ %s)" % (rel, k, want[k], got[i]))
        if got_err != want["error"]:
            problems.append("%s: header error differs (python '%s', C++ '%s')" % (rel, want["error"], got_err))
        cname = names.get(want["container"], "?")
        by_container[cname] = by_container.get(cname, 0) + 1
        key = "%s:%d" % (cname, want["unity_format"])
        by_format[key] = by_format.get(key, 0) + 1
        if want["expected_data_bytes"]:
            payload["match" if want["data_bytes"] == want["expected_data_bytes"] else "mismatch"] += 1
        if not want["loadable"]:
            unloadable.append({"path": rel, "error": want["error"]})
        tex_rows.append([rel, sha, len(data), want["container"], want["width"], want["height"], want["source_format"], want["unity_format"],
                         want["mip_chain"], want["mip_count"], want["is_normal_map"], want["loadable"], want["compressed"],
                         int(want["data_bytes"] == want["expected_data_bytes"]) if want["expected_data_bytes"] else -1])
    out = {"schema": "kspcore-wpue05-stock-sweep/1",
           "origin": "every file below is GameData/** of the dev copy decompiled/KSP-dev (KSP 1.12.5 build 03190); `path` is relative to its root; "
                     "derived data only (digests, counts, decoded headers), no game bytes",
           "digest_about": "per model: FNV-1a 64 over the compact rows of tools/wpue05/mu_render_reference.py (every value of every kept chunk; "
                           "large arrays as the FNV-1a 64 of their raw bytes); python and the C++ reader agree on every value",
           "summary": {"mu_files": len(mus), "mu_parsed_by_both": len(models), "mu_parse_errors": len(errors), "mu_versions": {str(k): v for k, v in sorted(versions.items())},
                       "chunk_totals": totals, "shader_names": dict(sorted(shaders.items())),
                       "ignored_node_entries": {str(k): v for k, v in sorted(ignored.items())},
                       "ignored_mesh_entries": {str(k): v for k, v in sorted(ignored_mesh.items())}, "warnings": warnings,
                       "textures": len(texs), "textures_by_container": by_container, "textures_by_container_and_unity_format": by_format,
                       "texture_payload_matches_expected": payload, "textures_not_loadable": unloadable,
                       "cross_check_problems": len(problems)},
           "parse_errors": errors,
           "model_fields": ["path", "sha256", "size", "version", "digest", "counts", "shaders"],
           "models": models,
           "texture_fields": ["path", "sha256", "size", "container", "width", "height", "source_format", "unity_format", "mip_chain", "mip_count",
                              "is_normal_map", "loadable", "compressed", "payload_matches_expected(-1 n/a)"],
           "textures": tex_rows}
    with open(args.out, "w", newline="\n") as f:
        json.dump(out, f, separators=(",", ":"))
    s = out["summary"]
    print("mu files %d, parsed by both %d, parse errors %d, versions %s" % (s["mu_files"], s["mu_parsed_by_both"], s["mu_parse_errors"], s["mu_versions"]))
    print("chunk totals %s" % totals)
    print("shaders %s" % s["shader_names"])
    print("ignored node entries %s, ignored mesh entries %s, warnings %d" % (s["ignored_node_entries"], s["ignored_mesh_entries"], len(warnings)))
    print("textures %d %s ; formats %s ; payload %s ; not loadable %d" % (s["textures"], by_container, by_format, payload, len(unloadable)))
    for pr in problems[:40]:
        print("PROBLEM:", pr)
    print("cross-check problems: %d" % len(problems))
    return 1 if problems or errors else 0


if __name__ == "__main__":
    sys.exit(main())
