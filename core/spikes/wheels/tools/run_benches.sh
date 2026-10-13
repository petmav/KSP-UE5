#!/usr/bin/env bash
# Wheel spike (core/WHEEL-SPIKE.md section 8): every measurement set into core/spikes/wheels/results/.
# Run from core/ after building (spikes/wheels/build_spike.ps1). Argument: the build tree (default build/spike-jolt).
# Pinned timing (cost) uses logical processor 2 of the 7950X3D's V-cache CCD, as the Jolt spike does.
set -euo pipefail
B=${1:-build/spike-jolt}
EXE="$B/spikes/wheels/wheel_spike.exe"
[ -x "$EXE" ] || EXE="$B/spikes/wheels/wheel_spike"
R=spikes/wheels/results
mkdir -p "$R"
"$EXE" info bug-e prospector twostage crawler > "$R/craft_info.txt"
"$EXE" info twostage --leg-offset > "$R/craft_info_legoffset.txt"
"$EXE" bench rest --warps 1,2,3,4 --out rest > /dev/null
"$EXE" bench drive --ground flat --warp4 --out drive_flat > /dev/null
"$EXE" bench drive --ground pqs --east -700 --out drive_pqs > /dev/null
"$EXE" bench drive --ground flat --g 0.166 --native explicit --friction smooth --out drive_mun_smooth > /dev/null
"$EXE" bench drive --ground flat --g 0.166 --native explicit --friction lineal --out drive_mun_lineal > /dev/null
"$EXE" bench brake --ground flat --out brake_flat > /dev/null
"$EXE" bench brake --ground flat --friction lineal --out brake_flat_lineal > /dev/null
"$EXE" bench brake --ground flat --crafts prospector --v0 8 --out brake_flat_prospector8 > /dev/null
"$EXE" bench touchdown --out touchdown > /dev/null
"$EXE" bench touchdown --leg-offset --out touchdown_legoffset > /dev/null
"$EXE" bench cost --affinity 4 --repeat 3 --seconds 20 --out cost_pinned > /dev/null
echo "results in $R"
