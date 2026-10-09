"""Turns spike/data/<tag>_summary.csv into Markdown tables for RESULTS.md.

Usage: python -I spike/analyze.py <tag>   (prints Markdown to stdout)
"""
import csv
import os
import sys

tag = sys.argv[1] if len(sys.argv) > 1 else 'full'
here = os.path.dirname(os.path.abspath(__file__))
rows = list(csv.DictReader(open(os.path.join(here, 'data', f'{tag}_summary.csv'), encoding='utf-8')))


def f(r, k, d=0.0):
    v = r.get(k, '')
    try:
        return float(v)
    except (TypeError, ValueError):
        return d


variants = [r for r in rows if r['phase'] not in ('Warmup', 'Stress')]
stress = [r for r in rows if r['phase'] == 'Stress']
warm = [r for r in rows if r['phase'] == 'Warmup']

print('### Image jitter proxy (GPU output, per-frame centroid of the marker cubes)\n')
print('A = 1 m cube with small primitive-local coordinates (how real objects are built). '
      'B = the same cube with vertices stored relative to the body centre (planet-centred mesh layout). '
      'Static/co-moving phases: deviation from the mean. GroundStrafe: residual after a quadratic fit of the motion. '
      'f2f = worst frame-to-frame step of that residual; pop = step across the forced origin shift.\n')
print('| variant / phase | A max dev px (cm) | A f2f px | A pop px | B max dev px (cm) | B f2f px | B pop px | Mun max dev px | frames |')
print('|---|---|---|---|---|---|---|---|---|')
for r in variants:
    if f(r, 'imgA_n') == 0 and f(r, 'imgMun_n') == 0:
        continue
    a = f'{f(r, "imgA_maxdev_px"):.3f} ({f(r, "imgA_maxdev_px") * f(r, "imgA_cm_per_px"):.3f})' if f(r, 'imgA_n') else '-'
    b = f'{f(r, "imgB_maxdev_px"):.3f} ({f(r, "imgB_maxdev_px") * f(r, "imgB_cm_per_px"):.2f})' if f(r, 'imgB_n') else '-'
    m = f'{f(r, "imgMun_maxdev_px"):.3f} (n={int(f(r, "imgMun_n"))}, {f(r, "imgMun_px_avg"):.0f} px)' if f(r, 'imgMun_n') else '-'
    print(f'| {r["label"]} | {a} | {f(r, "imgA_f2f_px"):.3f} | {f(r, "imgA_pop_px"):.3f} | {b} | {f(r, "imgB_f2f_px"):.3f} | '
          f'{f(r, "imgB_pop_px"):.3f} | {m} | {r["frames"]} |')

print('\n### Numeric jitter proxy (op-for-op emulation of the vertex path; max per-frame change of the screen-space error, px, on-screen points only)\n')
print('A_lwc / B_lwc: the cubes (or a virtual co-moving point 3 m ahead in non-image phases) through the UE 5.8 DF path. '
      'G_patch: a static ground vertex in a patch-local primitive (this spike\'s planet). G_planet: the same vertex in a planet-centred '
      'primitive. A_f32_*: a UE4/Unity-style float32 world-space pipeline, with the run\'s rebasing origin and with none.\n')
print('| variant / phase | A_lwc | B_lwc | G_patch | G_planet | A_f32 rebased | A_f32 no rebase | A_f32 no rebase (cm) |')
print('|---|---|---|---|---|---|---|---|')
for r in variants:
    print(f'| {r["label"]} | {f(r, "A_lwc_jit_max_px"):.4f} | {f(r, "B_lwc_jit_max_px"):.3f} | {f(r, "G_patch_jit_max_px"):.4f} | '
          f'{f(r, "G_planet_jit_max_px"):.3f} | {f(r, "A_f32_rebased_jit_max_px"):.4f} | {f(r, "A_f32_norebase_jit_max_px"):.3f} | '
          f'{f(r, "A_f32_norebase_jit_max_cm"):.2f} |')

print('\n### Frame times and origin shifts per phase (ms; image phases include a per-frame GPU readback)\n')
print('| variant / phase | frames | frame avg | frame p95 | frame max | game | render | rhi | gpu | shifts | shift avg | shift max | LOD total | patches | cam-origin max km |')
print('|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|')
for r in warm + variants:
    print(f'| {r["label"]} | {r["frames"]} | {f(r, "frame_ms_avg"):.2f} | {f(r, "frame_ms_p95"):.2f} | {f(r, "frame_ms_max"):.1f} | '
          f'{f(r, "game_ms_avg"):.2f} | {f(r, "render_ms_avg"):.2f} | {f(r, "rhi_ms_avg"):.2f} | {f(r, "gpu_ms_avg"):.2f} | {r["shifts"]} | '
          f'{f(r, "shift_ms_avg"):.3f} | {f(r, "shift_ms_max"):.3f} | {f(r, "lod_ms_total"):.0f} | {r["patches_max"]} | {f(r, "cam_from_origin_km_max"):.0f} |')

print('\n### Shift every tick (camera in a 100 km orbit at 2,246 m/s, 10 s each)\n')
print('Game-thread delta = game ms with shifting minus game ms of the no-shift baseline in the same scene.\n')
print('| scene / mode | frames | frame avg | game avg | game delta | render avg | gpu avg | shift ms avg | p50 | max | origin check cm | physics drift cm |')
print('|---|---|---|---|---|---|---|---|---|---|---|---|')
base = {}
for r in stress:
    if r['mode'] == 'None':
        base[r['variant']] = f(r, 'game_ms_avg')
for r in stress:
    d = f(r, 'game_ms_avg') - base.get(r['variant'], 0.0)
    print(f'| {r["label"]} | {r["frames"]} | {f(r, "frame_ms_avg"):.2f} | {f(r, "game_ms_avg"):.2f} | {d:+.2f} | {f(r, "render_ms_avg"):.2f} | '
          f'{f(r, "gpu_ms_avg"):.2f} | {f(r, "shift_ms_avg"):.3f} | {f(r, "shift_ms_p50"):.3f} | {f(r, "shift_ms_max"):.3f} | '
          f'{f(r, "shift_valid_err_cm"):.4f} | {f(r, "phys_drift_cm"):.4f} |')

print('\n### Memory\n')
mx = max(rows, key=lambda r: f(r, 'mem_peak_mb'))
print(f'Process working set: {f(rows[0], "mem_used_mb"):.0f} MB after warm-up, {max(f(r, "mem_used_mb") for r in rows):.0f} MB max at a step end, '
      f'peak {f(mx, "mem_peak_mb"):.0f} MB. RHI texture memory (streaming + non-streaming): {max(f(r, "tex_mb") for r in rows):.0f} MB.')
