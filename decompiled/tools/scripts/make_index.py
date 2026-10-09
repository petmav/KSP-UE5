"""Generate KSP-Decompiled/INDEX.md from logs/decompile-all-*.csv (written by decompile_all.ps1) plus
live .cs/line counts of every decompilation directory. Line count = number of '\n' bytes (same as `wc -l`).
Usage: python tools/scripts/make_index.py
"""
import csv
import datetime
import glob
import os

ROOT = r"C:\Projects\KSP-UE5\decompiled"
REPO = os.path.join(ROOT, "KSP-Decompiled")
LOGS = os.path.join(ROOT, "logs")
ACS = os.path.join(REPO, "Assembly-CSharp")

# Earlier (pre-existing) decompilations, not produced by decompile_all.ps1
SQUAD_DIRS = {
    r"KSP_x64_Data\Managed\Assembly-CSharp.dll": "Assembly-CSharp",
    r"KSP_x64_Data\Managed\Assembly-CSharp-firstpass.dll": "Assembly-CSharp-firstpass",
    r"KSP_x64_Data\Managed\KSPAssets.dll": "KSPAssets",
    r"KSP_x64_Data\Managed\KSPAssets.XmlSerializers.dll": "KSPAssets.XmlSerializers",
    r"KSP_x64_Data\Managed\KSPTrackIR.dll": "KSPTrackIR",
    r"GameData\Squad\Plugins\KSPSteamCtrlr.dll": "KSPSteamCtrlr",
}
ORIGIN_ORDER = {"Squad": 0, "third-party": 1, "Mono": 2, "Unity": 3, "native": 4}
NATIVE_DESC = {
    "KSP_x64.exe": "Unity player executable (game)",
    "Launcher.exe": "Unity player executable (launcher)",
    "UnityPlayer.dll": "Unity 2019.4.18f1 engine runtime (C++)",
    "UnityCrashHandler64.exe": "Unity crash handler",
    "mono-2.0-bdwgc.dll": "Mono runtime (MonoBleedingEdge, Boehm GC)",
    "MonoPosixHelper.dll": "native helper for Mono.Posix",
    "steam_api64.dll": "Steamworks SDK (used via Steamworks.NET in Assembly-CSharp-firstpass)",
    "lingoona_grammar_kerbal.dll": "Lingoona grammar engine (localization), used via Lingoona wrapper in firstpass",
}


def count_dir(d, skip_build_dirs=False):
    files = lines = 0
    for dp, dns, fns in os.walk(d):
        if skip_build_dirs and os.path.normcase(dp) == os.path.normcase(d):
            dns[:] = [x for x in dns if x.lower() not in ("bin", "obj")]
        for fn in fns:
            if fn.endswith(".cs"):
                files += 1
                with open(os.path.join(dp, fn), "rb") as fh:
                    lines += fh.read().count(b"\n")
    return files, lines


def count_file(p):
    with open(p, "rb") as fh:
        return fh.read().count(b"\n")


def read_csv(p):
    with open(p, encoding="utf-8-sig", newline="") as fh:
        return list(csv.DictReader(fh))


def fmt(n):
    return f"{n:,}" if isinstance(n, int) else n


def main():
    inv = read_csv(os.path.join(LOGS, "decompile-all-inventory.csv"))
    grp = {}
    for g in ("Unity", "Mono", "ThirdParty", "Launcher", "Expansions"):
        p = os.path.join(LOGS, f"decompile-all-{g}.csv")
        if os.path.exists(p):
            for r in read_csv(p):
                grp[r["source"].lower()] = r

    rows = []
    for r in inv:
        path = r["path"]
        e = dict(path=path, name=r["name"], ext=r["ext"], origin=r["origin"], version=r["version"],
                 obf=r["obfuscator"], dup=r["duplicateOf"], notes=[])
        if r["managed"] == "n":
            e.update(out="native, not decompilable", files="-", lines="-", build="n/a", obfy="-")
            e["notes"].append(NATIVE_DESC.get(os.path.basename(path), "native"))
        else:
            e["obfy"] = "n" if r["obfuscator"] == "none" else "y (" + r["obfuscator"] + ")"
            if path in SQUAD_DIRS:
                d = SQUAD_DIRS[path]
                f, l = count_dir(os.path.join(REPO, d), skip_build_dirs=(d == "Assembly-CSharp"))
                e.update(out=d + "\\", files=f, lines=l, build="**yes**" if d == "Assembly-CSharp" else "no")
                e["notes"].append("C# 7.3")
                if d == "Assembly-CSharp":
                    e["notes"].append("de4dot --dont-rename first; 23 compile fixes (FIXES.md); built DLL is symlinked into KSP-dev")
                    e["origin"] = "Squad"
                if d == "Assembly-CSharp-firstpass":
                    e["notes"].append("Squad-shipped, mostly third-party content (Steamworks.NET, Vectrosity, ...)")
            else:
                g = grp.get(path.lower())
                if g is None:
                    e.update(out="NOT DECOMPILED", files="-", lines="-", build="no")
                elif g["status"] not in ("ok", "exists"):
                    e.update(out=f"SKIPPED ({g['status']})", files="-", lines="-", build="no")
                else:
                    f, l = count_dir(os.path.join(REPO, g["outDir"]))
                    e.update(out=g["outDir"] + "\\", files=f, lines=l, build="no")
                    e["notes"].append("C# 7.3" if g["lang"] == "7.3" else "C# latest (11.0)")
                    if path.lower() == "launcher_data\\managed\\assembly-csharp.dll":
                        e["notes"].append("Squad launcher app (news, settings, graphics profiles); readable names")
                    if path.lower() == "launcher_data\\managed\\assembly-csharp-firstpass.dll":
                        e["notes"].append("EZ GUI / SpriteManager 2 UI library (third-party content)")
                    if g["deobfuscated"] == "y":
                        e["notes"].append("de4dot --dont-rename first")
            if r["duplicateOf"]:
                e["notes"].append("byte-identical to game copy")
        rows.append(e)

    def section(prefix_test):
        sel = [e for e in rows if prefix_test(e["path"])]
        sel.sort(key=lambda e: (ORIGIN_ORDER.get(e["origin"], 9), e["name"].lower()))
        return sel

    sections = [
        ("Game: `KSP_x64_Data\\Managed\\`", section(lambda p: p.startswith("KSP_x64_Data\\Managed\\"))),
        ("Game: `GameData\\` plugins", section(lambda p: p.startswith("GameData\\"))),
        ("Launcher: `Launcher_Data\\Managed\\`", section(lambda p: p.startswith("Launcher_Data\\Managed\\"))),
        ("Native (not .NET)", [e for e in rows if e["origin"] == "native"]),
    ]
    covered = set(id(e) for _, s in sections for e in s)
    leftovers = [e for e in rows if id(e) not in covered]
    if leftovers:
        sections.append(("Other managed files", leftovers))

    out = []
    w = out.append
    managed = [e for e in rows if e["origin"] != "native"]
    decomp = [e for e in managed if isinstance(e["files"], int)]
    tf = sum(e["files"] for e in decomp)
    tl = sum(e["lines"] for e in decomp)
    w("# INDEX - every assembly shipped with KSP 1.12.5 (build 03190, Unity 2019.4.18f1)")
    w("")
    w(f"Generated {datetime.date.today().isoformat()} by `tools\\scripts\\make_index.py` from `logs\\decompile-all-*.csv` "
      "(written by `tools\\scripts\\decompile_all.ps1`) plus live counts of the decompiled trees. ")
    w("Source install: `C:\\Projects\\KSP-UE5\\decompiled\\KSP-dev\\` (robocopy of the Steam install; the file lists are identical "
      "apart from KSP-dev's symlinked `Assembly-CSharp.dll/.pdb`, `KSP.log` and `settings.cfg`).")
    w("")
    w("- **Decompiler:** ICSharpCode.Decompiler 8.2.0.7535 via `tools\\IlspyExport` (legacy csproj, nested namespace directories). "
      "C# 7.3 for Squad and third-party assemblies, C# latest (= 11.0 in 8.2) for Unity and Mono BCL assemblies.")
    w("- **Obfuscated:** `de4dot -d` (v3.2.0) on every managed file. Only the game's `Assembly-CSharp.dll` has a known "
      "obfuscator (Crypto Obfuscator); de4dot reported \"Unknown Obfuscator\" (= none detected) for all others. "
      "For the game's Assembly-CSharp the stock original (`backup\\Assembly-CSharp.dll`) was probed, since the Managed copy is now a symlink to our build.")
    w("- **Lines** = newline count over the `.cs` files (`wc -l`). **Buildable**: only `Assembly-CSharp\\` is built (0 errors); "
      "everything else is read-only reference source and was never compiled.")
    w("- Paths in the *Decompiled path* column are relative to `KSP-Decompiled\\`.")
    w("- **Reproduce** (tools live outside this repo, in `C:\\Projects\\KSP-UE5\\decompiled\\tools\\`): "
      "`powershell -NoProfile -ExecutionPolicy Bypass -File tools\\scripts\\decompile_all.ps1 [-Group Unity,Mono,ThirdParty,Launcher,Expansions] [-Force]`, "
      "then the same script with `-Inventory`, then `python tools\\scripts\\make_index.py`. "
      "The earlier Squad decompilations (Assembly-CSharp, firstpass, KSPAssets*, KSPTrackIR, KSPSteamCtrlr) are documented in `FIXES.md` / `..\\STATUS.md`.")
    w(f"- **Totals:** {len(rows)} .dll/.exe files in the install: {len(managed)} managed, "
      f"{len(rows) - len(managed)} native. {len(decomp)} managed assemblies decompiled, "
      f"{fmt(tf)} .cs files, {fmt(tl)} lines.")
    w("")
    w("| Section | Files (.dll/.exe) | Managed | Native | Decompiled | .cs files | Lines |")
    w("|---|---|---|---|---|---|---|")
    for title, sel in sections:
        m = [e for e in sel if e["origin"] != "native"]
        d = [e for e in m if isinstance(e["files"], int)]
        w(f"| {title} | {len(sel)} | {len(m)} | {len(sel) - len(m)} | {len(d)} | {fmt(sum(e['files'] for e in d))} | {fmt(sum(e['lines'] for e in d))} |")
    by_origin = {}
    for e in decomp:
        o = by_origin.setdefault(e["origin"], [0, 0, 0])
        o[0] += 1; o[1] += e["files"]; o[2] += e["lines"]
    w("")
    w("| Origin | Assemblies decompiled | .cs files | Lines |")
    w("|---|---|---|---|")
    for o in sorted(by_origin, key=lambda k: ORIGIN_ORDER.get(k, 9)):
        a, f, l = by_origin[o]
        w(f"| {o} | {a} | {fmt(f)} | {fmt(l)} |")
    w("")
    dups = [e for e in rows if e["dup"]]
    w(f"{len(dups)} of the launcher's managed DLLs are byte-identical (sha256) to the game's copies; they were still "
      "decompiled separately under `Launcher\\` (git stores identical blobs once).")
    w("")
    w("No DLC: `GameData\\SquadExpansion\\` does not exist in KSP-dev or in the Steam install (checked read-only), so there are no "
      "expansion plugin DLLs and no `Expansions\\` output directory. The Making History / Breaking Ground *code* is compiled into "
      "Assembly-CSharp (`Assembly-CSharp\\Expansions\\`). `GameData\\Squad\\Plugins\\` contains only KSPSteamCtrlr.dll.")
    w("")
    for title, sel in sections:
        w(f"## {title}")
        w("")
        w("| Assembly | Origin | Version | Obfuscated | Decompiled path | .cs files | Lines | Buildable | Notes |")
        w("|---|---|---|---|---|---|---|---|---|")
        for e in sel:
            name = e["name"] + e["ext"] if e["origin"] == "native" else e["name"]
            if e["origin"] == "native":
                name = e["path"]
            w(f"| {name} | {e['origin']} | {e['version'] or '-'} | {e['obfy']} | `{e['out']}` | {fmt(e['files'])} | {fmt(e['lines'])} | {e['build']} | {'; '.join(e['notes'])} |"
              if not e["out"].startswith("native") else
              f"| {name} | native | - | - | native, not decompilable | - | - | n/a | {'; '.join(e['notes'])} |")
        w("")

    # ---- where to find things ----
    w("## Where to find things (core gameplay code for a UE5 port)")
    w("")
    w("All paths are relative to `KSP-Decompiled\\`. KSP's own gameplay classes are almost all in the **global namespace**, "
      "so they sit directly in `Assembly-CSharp\\` (1,394 files at the top level); sub-namespaces map to sub-directories. "
      "Line counts are for the current (fixed, buildable) tree.")
    w("")
    w("| Topic | Path | Files | Lines |")
    w("|---|---|---|---|")

    def f1(topic, rel):
        p = os.path.join(REPO, rel)
        if os.path.isfile(p):
            w(f"| {topic} | `{rel}` | 1 | {fmt(count_file(p))} |")
        else:
            w(f"| {topic} | `{rel}` | NOT FOUND | - |")

    def fd(topic, rel, note=""):
        p = os.path.join(REPO, rel)
        if os.path.isdir(p):
            f, l = count_dir(p)
            w(f"| {topic} | `{rel}\\`{note} | {fmt(f)} | {fmt(l)} |")
        else:
            w(f"| {topic} | `{rel}\\` | NOT FOUND | - |")

    def fg(topic, pattern):
        ps = sorted(glob.glob(os.path.join(REPO, pattern), recursive=True))
        for p in ps:
            w(f"| {topic} | `{os.path.relpath(p, REPO)}` | 1 | {fmt(count_file(p))} |")
        if not ps:
            w(f"| {topic} | `{pattern}` | NOT FOUND | - |")

    core = [
        ("Part (physics object, part tree)", "Part.cs"), ("Vessel", "Vessel.cs"), ("PartModule base", "PartModule.cs"),
        ("PartJoint (ConfigurableJoint wrapper)", "PartJoint.cs"), ("Orbit (Keplerian elements, propagation)", "Orbit.cs"),
        ("OrbitDriver (rails / physics sync)", "OrbitDriver.cs"), ("PatchedConicSolver", "PatchedConicSolver.cs"),
        ("Krakensbane (velocity frame shift)", "Krakensbane.cs"), ("FloatingOrigin", "FloatingOrigin.cs"),
        ("FlightIntegrator (aero, thermal, drag)", "FlightIntegrator.cs"), ("TimeWarp", "TimeWarp.cs"),
        ("PQS (procedural quad-sphere terrain)", "PQS.cs"), ("CelestialBody", "CelestialBody.cs"),
        ("FlightGlobals", "FlightGlobals.cs"), ("ConfigNode (cfg parser/serializer)", "ConfigNode.cs"),
        ("GameEvents", "GameEvents.cs"), ("ShipConstruct", "ShipConstruct.cs"), ("EditorLogic (VAB/SPH)", "EditorLogic.cs"),
    ]
    for t, f in core:
        f1(t, "Assembly-CSharp\\" + f)
    pqs = sorted(glob.glob(os.path.join(ACS, "PQSMod*.cs")))
    w(f"| PQS mods (terrain generators) | `Assembly-CSharp\\PQSMod*.cs` ({len(pqs)} files, global namespace) | {len(pqs)} | {fmt(sum(count_file(p) for p in pqs))} |")
    pqs_other = sorted(glob.glob(os.path.join(ACS, "PQS*.cs")))
    pqs_other = [p for p in pqs_other if not os.path.basename(p).startswith("PQSMod") and os.path.basename(p) != "PQS.cs"]
    w(f"| other PQS* support classes (PQSCity, PQSLandControl, quads, cache, ...) | `Assembly-CSharp\\PQS*.cs` minus the above | {len(pqs_other)} | {fmt(sum(count_file(p) for p in pqs_other))} |")
    fg("Engines", "Assembly-CSharp\\ModuleEngines*.cs")
    fg("Wheels (base)", "Assembly-CSharp\\ModuleWheel*.cs")
    fd("Wheels (submodules, namespace ModuleWheels)", "Assembly-CSharp\\ModuleWheels")
    w("")
    w("| Namespace / subsystem | Path | Files | Lines | Notes |")
    w("|---|---|---|---|---|")
    ns = [
        ("CommNet (relay network)", "Assembly-CSharp\\CommNet", "Squad"),
        ("Contracts", "Assembly-CSharp\\Contracts", "Squad"),
        ("FinePrint (stock contract types)", "Assembly-CSharp\\FinePrint", "Squad; originally a mod by Arsonide"),
        ("Expansions (all)", "Assembly-CSharp\\Expansions", "Squad DLC code compiled into the base game"),
        ("Expansions\\Missions = Making History", "Assembly-CSharp\\Expansions\\Missions", "mission editor/runtime"),
        ("Expansions\\Serenity = Breaking Ground", "Assembly-CSharp\\Expansions\\Serenity", "robotics, deployed science"),
        ("KSP\\UI", "Assembly-CSharp\\KSP\\UI", "Squad UI"),
        ("VehiclePhysics", "Assembly-CSharp\\VehiclePhysics", "third-party embedded: Edy's Vehicle Physics (drivetrain, tire/suspension model); used by KSPWheelController.cs, ModuleWheelBase.cs, Vessel.cs"),
        ("EdyCommonTools", "Assembly-CSharp\\EdyCommonTools", "third-party embedded: Edy's common tools"),
        ("LibNoise", "Assembly-CSharp\\LibNoise", "third-party embedded: LibNoise (C# port), used by PQS"),
        ("TMPro", "Assembly-CSharp\\TMPro", "third-party embedded: TextMesh Pro"),
        ("Smooth", "Assembly-CSharp\\Smooth", "third-party embedded: Smooth.Foundations (Smooth.Algebraics, Smooth.Slinq, ...)"),
        ("SoftMasking", "Assembly-CSharp\\SoftMasking", "third-party embedded: Soft Mask for Unity UI"),
        ("UniLinq", "Assembly-CSharp\\UniLinq", "third-party embedded: LINQ reimplementation (AOT-safe)"),
        ("Highlighting", "Assembly-CSharp\\Highlighting", "third-party embedded: Highlighting System"),
    ]
    for t, rel, note in ns:
        p = os.path.join(REPO, rel)
        if os.path.isdir(p):
            f, l = count_dir(p)
            w(f"| {t} | `{rel}\\` | {fmt(f)} | {fmt(l)} | {note} |")
        else:
            w(f"| {t} | `{rel}\\` | NOT FOUND | - | {note} |")
    w("")
    w("Unity-side API that the gameplay code calls (physics, joints, transforms) is under `Unity\\`, e.g. "
      "`Unity\\UnityEngine.PhysicsModule\\UnityEngine\\` (Rigidbody, ConfigurableJoint, Physics, colliders), "
      "`Unity\\UnityEngine.VehiclesModule\\UnityEngine\\WheelCollider.cs` and "
      "`Unity\\UnityEngine.CoreModule\\UnityEngine\\` (Transform, Vector3, Quaternion). KSP's double-precision math types are its own: "
      "`Assembly-CSharp\\Vector3d.cs`, `Assembly-CSharp\\Matrix4x4D.cs`, `Assembly-CSharp\\UnityEngine\\QuaternionD.cs` "
      "(declared in namespace UnityEngine inside Assembly-CSharp), `Assembly-CSharp\\Planetarium.cs` (inertial frame, UT). "
      "Note these Unity assemblies are thin managed bindings: most members are `extern` / `[MethodImpl(MethodImplOptions.InternalCall)]` "
      "into the native engine (UnityPlayer.dll, incl. PhysX), so the decompiled Unity code documents the API surface, not the engine behaviour.")
    w("")
    with open(os.path.join(REPO, "INDEX.md"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(out))
    print(f"INDEX.md written: {len(rows)} files, {len(managed)} managed, {len(decomp)} decompiled, {tf} .cs, {tl} lines")
    for title, sel in sections:
        m = [e for e in sel if e["origin"] != "native"]
        d = [e for e in m if isinstance(e["files"], int)]
        print(f"  {title}: managed {len(m)}, decompiled {len(d)}, files {sum(e['files'] for e in d)}, lines {sum(e['lines'] for e in d)}")


if __name__ == "__main__":
    main()
