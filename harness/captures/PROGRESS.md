# Captures (C1/C2/C7/C3) progress log

Agent: Opus implementer, started 2026-10-09.

## State
- Read: CLAUDE.md, NEXT-STEPS, CAPTURE-SPEC, harness README, physics PROGRESS/PROFILE, STATUS.
- Repo: main @ 4e54f682 (checked out, clean), profiling @ 10006ec2.

## Plan
1. Worktree for profiling at decompiled/KSP-Decompiled-profiling (as previous agent did); add Capture mode to PhysProfDriver (cmdline -harness-capture <list>).
2. C1 curves_eval_native, C2 unity_random, C7 libm_mono, C3 math_shim_native.
3. Build Optimize=true, copy DLL over GameData/Assembly-CSharp-cleaned, launch, collect, compare.
4. Restore main, rebuild, launch check.

## Log
- Pre-state: symlinked DLL sha256 B484F33D... (main build-08). No KSP running. Checked out `profiling` in the main checkout (no worktree; build output lands directly in the symlink target). MUST checkout main + Rebuild at end.
- Design: PhysProfDriver.Start dispatches `-harness-capture <items> -harness-capture-in <dir> -harness-capture-out <dir>` to new files in Assembly-CSharp/PhysProf/:
  HarnessCaptureJson.cs (Unity-free ksp-golden/1 writer, hex cols auto), HarnessCaptureC7.cs (Unity-free libm/f32/conv/tostring/parse/System.Random kernel, also compiled into netfx ref tool), HarnessCapture.cs (C1, C2, C3, math_shim replica, provenance).
- Inputs prepared by harness/captures/tools/prep_inputs.py into harness/captures/in/; netfx ref tool in harness/captures/tools/netfx (Roslyn csc, x64, /optimize+) generates System.Random(7)/(11) inputs + libm_dotnet.json.
- Launch: decompiled/tools/scripts/capture_ksp.ps1 (from profile_ksp.ps1), logs decompiled/logs/ksp-capture-<tag>-*.log.
- Build: decompiled/tools/scripts/build_opt.sh <tag> (build.sh + -p:Optimize=true).
- 00:56 Built profiling+capture with Optimize=true (build-capture-01, 0 errors, sha 06c8076b...). Committed ecd026a5 on profiling.
- Inputs prepared: in/curves_eval_in.tsv (366 curves, 28484 samples), parse_tokens.tsv (296), libm_base_inputs.tsv (2064), libm_inputs.tsv (3093), f32_inputs.tsv (10000).
- .NET reference: tools/netfx/bin/LibmRef.exe -> out/libm_dotnet.json (228,966 rows; decimals verified vs hex).
- Next: first launch `powershell -File decompiled/tools/scripts/capture_ksp.ps1 -Tag run1`.
- 00:58 run1: all 6 captures OK in 48 s, 0 exceptions. C1 BIT-EXACT (366 curves/1055 keys/28484 samples). math_shim_mono: f64 exact except live Zup; f32 1-6 ulp diffs (Mono double intermediates) + signed-zero Atan2/Pow. C7: only Math.Tan differs among transcendentals (Mono more accurate); Ceiling(-0.5) sign; Mono float32 = double intermediates, rounding at stores/conv.r4; (ulong)NaN; ulong->double; one ToString G15 mis-round in Mono; parse+System.Random identical.
- 01:10 added arg/return/store probes (commit after ecd026a5), build-capture-02 sha 5ea03c58...; netfx rebuilt. Next: run2 capture all, analyze, then C2 random model, C3 summary.
- 01:13 run3 (commit 5fedcfab, DLL d87eee7b...): all 7 captures OK (adds aero_forces_mono + trace_vec), 51 s, 0 exceptions.
- normalize.py (decimals from hex via .NET G-format candidates from LibmRef fmt) makes compare.py join float keys; C1 compare.py PASS with max_abs=0.
- RESULTS so far: C1 BIT-EXACT. C7: libm identical except Math.Tan (Mono <=1ulp accurate; .NET 94 cases >1ulp, NaN for |x|>=1e22), Ceiling(-x) sign, Atan2(-0) sign, Pow(-0,-3) sign; Mono float32: double intermediates, rounded at every store (locals/args/returns/fields/statics/arrays/conv.r4), `double r=a*b` exact. conv (ulong)NaN etc 0x8000.. vs 0; (double)ulong .NET mis-rounds; Mono ToString() G15 mis-round 1/600; parse/System.Random identical. aero under Mono: drag_curve_value up to 61 ulp, cube_drag up to 37 ulp, CalculateDragValue rel 3e-6 (exact product). math_shim f32 up to 25 ulp, Mathf.Repeat(1e20,360) 0 vs 360. C2: InitState/step/value/Range(int)/Range(float)=v*min+(1-v)*max all bit-exact; GetClassRadius 45/45; vector fns pending (trace_vec).
- Next: fit vector fns (trace_vec), C3 accuracy summary, README, restore main.
- 01:25 C2 vector fns fitted (tools/random_vec_fit.py, reports/C2_vector_fit.md): draws per call insideUnitSphere 3, onUnitSphere 2, rotation 4, rotationUniform 3, insideUnitCircle 2, ColorHSV 4. onUnitSphere: z=Range(-1,1), a=Range(0,2pi), r=sqrtf(1-z*z), (r cos a, r sin a, z) BIT-EXACT; insideUnitSphere = onUnitSphere * powf(value,1/3) BIT-EXACT; rotation = Normalize(4 x Range(-1,1)) then negate if w<0 BIT-EXACT; rotationUniform = Shoemake (u1=Range(0,1), u2,u3=Range(0,2pi); (a sin u2, a cos u2, b sin u3, b cos u3), a=sqrt(1-u1), b=sqrt(u1)) then w>=0 BIT-EXACT; insideUnitCircle theta=Range(0,2pi), r=sqrt(Range(0,1)) (r cos, r sin) 95/96 (1 ulp: CRT sinf near-tie).
- Next: C3 reference check (tools/c3_check.py), README, cleanup (_patch_run3.py, _aero_snippet.cs), restore main.
- 01:23 C3 convention check done (reports/C3_check.md). run_capture.sh wrapper tested end to end (`bash harness/captures/run_capture.sh all final`, 87 s); outputs bit-identical to run3 (deterministic). Helper patch files removed.
- Next: README.md, then restore main (checkout main, build.sh Rebuild, launch_ksp.ps1 check, DLL hash check), final report.
- 01:28 DONE. README.md written. netfx src refreshed (SOURCE.txt: profiling @ 5fedcfab), libm_dotnet regenerated (bit-identical).
- Restore: `main` checked out (4e54f682, clean); build-09-post-captures-main Rebuild (build.sh, /optimize-): 0 errors, 1454 warnings, 9,866,240 bytes, sha256 42023ed1..., no HarnessCapture/PhysProf strings. launch_ksp.ps1 -Tag post-captures-main: MAINMENU 39 s, 0 exceptions (KSP.log + Player.log), killed. Symlink hash == build output. No KSP process running.
- settings.cfg vs backup/settings.cfg.pre-profiling: only SHOW_WHATSNEW_DIALOG flags differ (KSP rewrites at launch); resolution not persisted.
- profiling branch head: 5fedcfab (commits ecd026a5, ade40c4c, 5fedcfab added this session). New scripts outside the repo: decompiled/tools/scripts/build_opt.sh, capture_ksp.ps1.
