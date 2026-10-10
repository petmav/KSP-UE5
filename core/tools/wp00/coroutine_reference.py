#!/usr/bin/env python3
"""WP-00 independent replay of the coroutine scheduler model (numpy float32), writes tests/data/wp00_reference.json.

Stimuli are NOT hand-authored: the per-frame delta times (float32 of frame_ms / 1000) and the number of fixed steps per frame are the
first FRAMES rows of the in-game physics profile analysis/physics/raw/run1/run1-L_ascent_frames.csv (columns frame_ms, n_fixed,
fixed_dt). The replay is a direct, event-style computation of the semantics documented in include/ksp/core/coroutine.hpp (it does not
simulate queues), so it is independent of the C++ scheduler:

  clock t_k = t_(k-1) + float64(dt_k)            (double, frame k = 1..FRAMES; t_0 = 0)
  Time.time in Update/dynamic pass = float32(t_k); Time.deltaTime = dt_k (float32)
  fixed step g (1-based, global): fixedTime_g = fixedTime_(g-1) + float64(fixed_dt_f32); Time.time = float32(fixedTime_g)
  WaitForSeconds(s) yielded at clock c (frame y): wake = c + float64(float32(s)); resumes in the first dynamic pass of a frame k > y
      (a pass works on a snapshot) with t_k >= wake; for the start (before frame 1) y = 0.
  yield null: resumes in frame y+1 (k > y).
  WaitForFixedUpdate: resumes after the next fixed step (coroutines start before frame 1, so step g resumes after step g-1).
  WaitForEndOfFrame: resumes in the end-of-frame pass of frame y+1 for a routine started before frame 1 / resumed in the previous
      end-of-frame pass.

Output records [frame, f32 bits of Time.time (or of the float accumulator), extra]. Run: python -I coroutine_reference.py
"""
import csv
import json
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
FRAMES_CSV = os.path.join(ROOT, "analysis", "physics", "raw", "run1", "run1-L_ascent_frames.csv")
if not os.path.isfile(FRAMES_CSV):  # worktree without the analysis tree checked out next to it
    FRAMES_CSV = r"C:\Projects\KSP-UE5\analysis\physics\raw\run1\run1-L_ascent_frames.csv"
OUT = os.path.join(ROOT, "core", "tests", "data", "wp00_reference.json")
FRAMES = 600


def bits(x):
    return struct.unpack("<I", struct.pack("<f", float(np.float32(x))))[0]


def main():
    rows = list(csv.DictReader(open(FRAMES_CSV)))[:FRAMES]
    dt = [np.float32(float(r["frame_ms"]) / 1000.0) for r in rows]
    nfix = [int(r["n_fixed"]) for r in rows]
    fixed_dt = np.float32(float(rows[0]["fixed_dt"]))

    t = []
    c = 0.0
    for d in dt:
        c = c + float(d)
        t.append(c)
    step_frame = []
    step_time = []
    ft = 0.0
    for k, n in enumerate(nfix, start=1):
        for _ in range(n):
            ft = ft + float(fixed_dt)
            step_frame.append(k)
            step_time.append(ft)

    def wait_chain(secs_list):
        rec = []
        y = 0
        clock = 0.0
        for s in secs_list:
            wake = clock + float(np.float32(s))
            k = y + 1
            while k <= FRAMES and not (t[k - 1] >= wake):
                k += 1
            if k > FRAMES:
                break
            rec.append([k, bits(np.float32(t[k - 1])), 0])
            y = k
            clock = t[k - 1]
        return rec

    out = {"frames": FRAMES, "fixed_dt_bits": bits(fixed_dt),
           "dt_bits": [bits(d) for d in dt], "n_fixed": nfix}
    out["w1"] = wait_chain([0.1] * 6)
    out["w2"] = wait_chain([0.25] * 4)
    out["w3"] = wait_chain([1.5] + [0.016] * 5)
    acc = np.float32(0.0)
    rec = []
    for k in range(1, 9):
        acc = np.float32(acc + dt[k - 1])
        rec.append([k, bits(acc), 0])
    out["n"] = rec
    out["f"] = [[step_frame[g - 1], bits(np.float32(step_time[g - 1])), g] for g in range(1, 7)]
    out["e"] = [[k, bits(np.float32(t[k - 1])), 0] for k in range(1, 6)]
    out["total_fixed_steps"] = len(step_frame)
    out["clock_end_bits"] = [bits(np.float32(t[-1]))]
    with open(OUT, "w") as f:
        json.dump(out, f, separators=(",", ":"))
    print("wrote", OUT, "fixed steps", len(step_frame), "t_end", t[-1])


if __name__ == "__main__":
    sys.exit(main())
