#!/usr/bin/env python3
"""Second manual pass (profiling branch): PQS coroutine sub-phases. Run after physprof_manual.py.
Usage: python physprof_manual2.py <Assembly-CSharp dir>"""
import os, sys
sys.argv = [sys.argv[0], sys.argv[1]]
ROOT = sys.argv[1]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "physprof_manual.py")).read().split("P = \"global::PhysProf.\"")[0])
P = "global::PhysProf."
p, L = load("PQS.cs")
r = find_method_range(L, r"^\tprivate IEnumerator UpdateSphere\(\)$")
wrap_stmt(L, "Mod_OnPreUpdate();", P + "PQS_ModOnPreUpdate", within=r)
wrap_stmt(L, "UpdateVisual();", P + "PQS_UpdateVisual", within=r)
r = find_method_range(L, r"^\tprivate IEnumerator UpdateSphere\(\)$")
wrap_stmt(L, "UpdateQuads();", P + "PQS_UpdateQuads", within=r)
r = find_method_range(L, r"^\tprivate IEnumerator UpdateSphere\(\)$")
wrap_stmt(L, "Mod_OnUpdateFinished();", P + "PQS_ModOnUpdateFinished", within=r)
save(p, L)
print("PQS instrumentation applied")
