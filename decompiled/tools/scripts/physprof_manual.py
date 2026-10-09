#!/usr/bin/env python3
"""Manual sub-phase instrumentation for the `profiling` branch (run after physprof_instrument.py).
Wraps selected statements and method bodies in PhysProf.Begin/End (try/finally).
Usage: python physprof_manual.py <Assembly-CSharp dir>
"""
import re
import sys
import os

ROOT = sys.argv[1]


def load(rel):
    p = os.path.join(ROOT, rel)
    t = open(p, "rb").read().decode("utf-8")
    return p, t.split("\r\n")


def save(p, lines):
    open(p, "wb").write("\r\n".join(lines).encode("utf-8"))


def wrap_stmt(lines, stmt, idexpr, expect=1, within=None):
    """Wrap every line whose stripped text == stmt (optionally only between line indices `within`)."""
    n = 0
    for i, l in enumerate(lines):
        if within and not (within[0] <= i < within[1]):
            continue
        if l.strip() == stmt:
            ind = l[: len(l) - len(l.lstrip("\t"))]
            lines[i] = ind + "{ long __ppm = global::PhysProf.Begin(); try { " + stmt + " } finally { global::PhysProf.End(" + idexpr + ", __ppm); } }"
            n += 1
    if n != expect:
        raise SystemExit("wrap_stmt %r: expected %d got %d" % (stmt, expect, n))


def wrap_method(lines, sigregex, idexpr):
    rx = re.compile(sigregex)
    for i, l in enumerate(lines):
        if rx.match(l):
            ind = l[: len(l) - len(l.lstrip("\t"))]
            assert lines[i + 1] == ind + "{", (sigregex, lines[i + 1])
            for k in range(i + 2, len(lines)):
                if lines[k] == ind + "}":
                    break
            else:
                raise SystemExit("no close for " + sigregex)
            new = lines[: i + 2] + [ind + "\tlong __ppm0 = global::PhysProf.Begin();", ind + "\ttry", ind + "\t{"] + lines[i + 2 : k] + \
                [ind + "\t}", ind + "\tfinally", ind + "\t{", ind + "\t\tglobal::PhysProf.End(" + idexpr + ", __ppm0);", ind + "\t}"] + lines[k:]
            lines[:] = new
            return
    raise SystemExit("method not found: " + sigregex)


def find_method_range(lines, sigregex):
    rx = re.compile(sigregex)
    for i, l in enumerate(lines):
        if rx.match(l):
            ind = l[: len(l) - len(l.lstrip("\t"))]
            for k in range(i + 2, len(lines)):
                if lines[k] == ind + "}":
                    return (i, k)
    raise SystemExit("range not found: " + sigregex)


P = "global::PhysProf."

# ---- FlightIntegrator
p, L = load("FlightIntegrator.cs")
fu = find_method_range(L, r"^\tprotected virtual void FixedUpdate\(\)$")
wrap_stmt(L, "UpdateMassStats();", P + "FI_UpdateMassStats", within=fu)
wrap_stmt(L, "DragCubeSetupAndPartAeroStats(vessel);", P + "FI_DragCubeSetup", within=fu)
wrap_stmt(L, "CheckThermalGraph();", P + "FI_CheckThermalGraph", within=fu)
wrap_stmt(L, "UpdateOcclusion(all: true);", P + "FI_OcclusionFixed", within=fu)
wrap_stmt(L, "UpdateThermodynamics();", P + "FI_UpdateThermodynamics", within=fu)
wrap_stmt(L, "IntegratePhysicalObjects(FlightGlobals.physicalObjects, density);", P + "FI_IntegratePhysicalObjects", within=fu)
wrap_stmt(L, "Integrate(partRef);", P + "FI_Integrate", within=fu)
up = find_method_range(L, r"^\tpublic virtual void Update\(\)$")
wrap_stmt(L, "UpdateOcclusion(all: false);", P + "FI_OcclusionUpdate", within=up)
wrap_method(L, r"^\tpublic virtual void ThermoPrecalculate\(\)$", P + "FI_ThermoPrecalculate")
wrap_method(L, r"^\tprotected virtual void UpdateAerodynamics\(Part part\)$", P + "FI_UpdateAerodynamics")
wrap_method(L, r"^\tpublic virtual void ThermalIntegrationPass\(bool averageWithPrevious\)$", P + "FI_ThermalIntegrationPass")
wrap_method(L, r"^\tpublic virtual void UpdateConduction\(\)$", P + "FI_UpdateConduction")
save(p, L)

# ---- VesselPrecalculate
p, L = load("VesselPrecalculate.cs")
wrap_method(L, r"^\tpublic virtual void CalculatePhysicsStats\(\)$", P + "VP_CalculatePhysicsStats")
wrap_method(L, r"^\tpublic virtual void CalculateGravity\(\)$", P + "VP_CalculateGravity")
wrap_method(L, r"^\tpublic virtual void MainPhysics\(bool doKillChecks\)$", P + "VP_MainPhysics")
save(p, L)

# ---- OrbitDriver
p, L = load("OrbitDriver.cs")
wrap_method(L, r"^\tpublic void UpdateOrbit\(bool offset = true\)$", P + "OD_UpdateOrbit")
save(p, L)

# ---- FloatingOrigin / Krakensbane
p, L = load("FloatingOrigin.cs")
wrap_method(L, r"^\tprotected void setOffset\(Vector3d refPos, Vector3d nonFrame\)$", P + "FO_SetOffset")
save(p, L)
p, L = load("Krakensbane.cs")
wrap_method(L, r"^\tpublic void AddExcess\(Vector3d vel\)$", P + "KB_AddExcess")
wrap_method(L, r"^\tpublic void Zero\(\)$", P + "KB_Zero")
save(p, L)

# ---- Part: per-module-type OnFixedUpdate / OnUpdate
p, L = load("Part.cs")
r = find_method_range(L, r"^\tpublic void ModulesOnFixedUpdate\(\)$")
n = 0
for i in range(r[0], r[1]):
    if L[i].strip() == "modules[i].OnFixedUpdate();":
        ind = L[i][: len(L[i]) - len(L[i].lstrip("\t"))]
        L[i] = (ind + "int __ppid = partModule.ppIdOnFixed; if (__ppid < 0) { __ppid = partModule.ppIdOnFixed = global::PhysProf.DynId(partModule.GetType().Name + \".OnFixedUpdate\"); } "
                "long __ppt = global::PhysProf.Begin(); try { modules[i].OnFixedUpdate(); } finally { global::PhysProf.End(__ppid, __ppt); }")
        n += 1
assert n == 1, n
r = find_method_range(L, r"^\tpublic void ModulesOnUpdate\(\)$")
n = 0
for i in range(r[0], r[1]):
    if L[i].strip() == "modules[i].OnUpdate();":
        ind = L[i][: len(L[i]) - len(L[i].lstrip("\t"))]
        L[i] = (ind + "PartModule __pm = modules[i]; int __ppid = __pm.ppIdOnUpdate; if (__ppid < 0) { __ppid = __pm.ppIdOnUpdate = global::PhysProf.DynId(__pm.GetType().Name + \".OnUpdate\"); } "
                "long __ppt = global::PhysProf.Begin(); try { __pm.OnUpdate(); } finally { global::PhysProf.End(__ppid, __ppt); }")
        n += 1
assert n == 1, n
wrap_method(L, r"^\tpublic void ModulesOnFixedUpdate\(\)$", P + "Part_ModulesOnFixedUpdate")
wrap_method(L, r"^\tpublic void ModulesOnUpdate\(\)$", P + "Part_ModulesOnUpdate")
save(p, L)

# ---- PartModule: cached dynamic ids
p, L = load("PartModule.cs")
for i, l in enumerate(L):
    if l.startswith("public class PartModule"):
        assert L[i + 1] == "{"
        L.insert(i + 2, "\t[NonSerialized]")
        L.insert(i + 3, "\tinternal int ppIdOnFixed = -1;")
        L.insert(i + 4, "")
        L.insert(i + 5, "\t[NonSerialized]")
        L.insert(i + 6, "\tinternal int ppIdOnUpdate = -1;")
        L.insert(i + 7, "")
        break
else:
    raise SystemExit("PartModule class not found")
save(p, L)
print("manual instrumentation applied")
