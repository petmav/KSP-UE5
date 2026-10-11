#!/usr/bin/env bash
# Run the measurement sets of core/PHYSICS-SPIKE.md one after the other (never in parallel: they are timings) and write
# results/<set>.md (the printed tables) and results/<set>.csv. Usage, from core/:
#   bash spikes/jolt/tools/run_benches.sh [build/spike-jolt] [set ...]
# Sets: scaling scaling_pad threads iterations warp shift precision pipeline staging orbit loads determinism capacity selftest
set -euo pipefail
BUILD=${1:-build/spike-jolt}
shift || true
EXE="$BUILD/spikes/jolt/jolt_spike.exe"
[ -x "$EXE" ] || EXE="$BUILD/spikes/jolt/jolt_spike"
OUT=spikes/jolt/results
mkdir -p "$OUT"
SUFFIX=""
case "$BUILD" in *f32*) SUFFIX="_f32" ;; esac
SETS=("$@")
[ ${#SETS[@]} -eq 0 ] && SETS=(scaling scaling_pad threads iterations warp shift precision pipeline staging orbit loads determinism capacity selftest)
run() {  # name, args...
    local name=$1; shift
    echo "== $name$SUFFIX: $*"
    "$EXE" bench "$@" --csv "$OUT/$name$SUFFIX.csv" > "$OUT/$name$SUFFIX.md" 2>&1
    tail -n +1 "$OUT/$name$SUFFIX.md" | head -60
}
for s in "${SETS[@]}"; do
    case "$s" in
        scaling) run scaling_ascent scaling ;;
        scaling_pad) run scaling_pad scaling --pad ;;
        threads) run threads threads ;;
        iterations) run iterations iterations --models articulated --crafts PhysProf-S,PhysProf-M2-AS,PhysProf-L2-AS,PhysProf-L2 --steps 4/2,10/2,20/2,30/2,50/2,50/10 ;;
        warp) run warp warp --crafts PhysProf-M2-AS,PhysProf-L2-AS ;;
        shift) run shift shift ;;
        precision) run precision precision ;;
        pipeline) run pipeline pipeline ;;
        staging) run staging staging ;;
        orbit) run orbit orbit --crafts PhysProf-M2-AS --models articulated,reduced-whole ;;
        loads) run loads loads ;;
        determinism) run determinism determinism ;;
        capacity) run capacity capacity ;;
        selftest) run selftest selftest ;;
        *) echo "unknown set $s"; exit 2 ;;
    esac
done
