#!/usr/bin/env bash
# One command per capture: inputs -> KSP launch (capture mode) -> normalize/verify -> compare/analysis.
#   bash harness/captures/run_capture.sh <c1|c2|c7|mathshim|aero|c3|qdextern|all> [tag]          batch 1 -> out/, reports/
#   bash harness/captures/run_capture.sh <c4|c5|c6|c8|c9|c10|terrain|batch2> [tag]                batch 2 -> out2/, reports2/
#   (batch-2 items can be combined with commas, e.g. c4,c6; they share one launch)
# Needs the `profiling` branch build of Assembly-CSharp in the KSP-dev symlink target (see README.md "Build").
set -e
ITEM=${1:?usage: run_capture.sh <c1|c2|c7|mathshim|aero|c3|qdextern|all|c4|c5|c6|c8|c9|c10|terrain|batch2> [tag]}
ROOT=/c/Projects/KSP-UE5
CAP=$ROOT/harness/captures
DLL=$ROOT/decompiled/KSP-Decompiled/GameData/Assembly-CSharp-cleaned/Assembly-CSharp.dll
PY="python -I"
if ! grep -q "HarnessCapture" "$DLL"; then
  echo "ERROR: the symlinked Assembly-CSharp.dll has no capture mode. Build the profiling branch first:"
  echo "  git -C $ROOT/decompiled/KSP-Decompiled checkout profiling && bash $ROOT/decompiled/tools/scripts/build_opt.sh capture-NN"
  exit 2
fi
cd "$ROOT"
case ",$ITEM," in
  *,c4,*|*,c5,*|*,c6,*|*,c8,*|*,c9,*|*,c10,*|*,terrain,*|*,batch2,*)
    if ! grep -q "HarnessCapture2" "$DLL"; then echo "ERROR: DLL predates batch 2 (no HarnessCapture2); rebuild the profiling branch"; exit 2; fi
    TAG=${2:-capture2-$(echo "$ITEM" | tr ',' '_')-$(date +%Y%m%d-%H%M%S)}
    mkdir -p $CAP/out2 $CAP/reports2
    # batch 2 needs no inputs; flight/editor sessions can take ~10 min (c10 launches three craft)
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Projects\KSP-UE5\decompiled\tools\scripts\capture_ksp.ps1' -Tag "$TAG" -Items "$ITEM" -OutDir 'C:\Projects\KSP-UE5\harness\captures\out2' -TimeoutSec 1800
    declare -A F2=([c4]=atmosphere_thermal_offsets [c5]=deltav_readout [c6]=thermal_step [c8]=orbit_rails_ingame [c9]=part_database [c10]=aero_forces_vessel [terrain]=terrain_height)
    ITEMS=$(echo "$ITEM" | tr ',' ' ')
    [ "$ITEM" = batch2 ] && ITEMS="c9 terrain c5 c6 c4 c8 c10"
    for it in $ITEMS; do
      f=$CAP/out2/${F2[$it]}.json
      [ -f "$f" ] || { echo "MISSING $f"; continue; }
      $PY $CAP/tools/normalize.py "$f"
      $PY $CAP/tools/verify_hex.py "$f"
      $PY $CAP/tools/batch2_report.py $it
    done
    exit 0 ;;
esac
TAG=${2:-$ITEM-$(date +%Y%m%d-%H%M%S)}
$PY $CAP/tools/prep_inputs.py
if [ ! -x $CAP/tools/netfx/bin/LibmRef.exe ]; then bash $CAP/tools/netfx/build.sh; fi
$CAP/tools/netfx/bin/LibmRef.exe gen $CAP/in
if [ "$ITEM" = c7 ] || [ "$ITEM" = all ]; then
  $CAP/tools/netfx/bin/LibmRef.exe run $CAP/in $CAP/out/libm_dotnet.json
fi
powershell.exe -NoProfile -ExecutionPolicy Bypass -File 'C:\Projects\KSP-UE5\decompiled\tools\scripts\capture_ksp.ps1' -Tag "$TAG" -Items "$ITEM" | grep -v "qdextern: calling"
declare -A FILE=([c1]=curves_eval_native [c2]=unity_random [c7]=libm_mono [mathshim]=math_shim_mono [aero]=aero_forces_mono [c3]=math_shim_native [qdextern]=quaterniond_externs)
ITEMS=$ITEM
[ "$ITEM" = all ] && ITEMS="c1 c7 mathshim c2 aero c3 qdextern"
for it in $ITEMS; do
  f=$CAP/out/${FILE[$it]}.json
  $PY $CAP/tools/normalize.py "$f"
  $PY $CAP/tools/verify_hex.py "$f"
  case $it in
    c1)       $PY harness/compare/compare.py harness/golden/curves_eval/curves_eval.json "$f" --report $CAP/reports/C1_compare.json | tail -1
              $PY $CAP/tools/bitcompare.py harness/golden/curves_eval/curves_eval.json "$f" --json $CAP/reports/C1_bitcompare.json | tail -1 ;;
    c7)       $PY $CAP/tools/normalize.py $CAP/out/libm_dotnet.json
              $PY $CAP/tools/bitcompare.py $CAP/out/libm_dotnet.json "$f" --json $CAP/reports/C7_bitcompare.json | tail -1
              $PY $CAP/tools/analyze_c7.py --md $CAP/reports/C7_analysis.md > /dev/null && echo "wrote reports/C7_analysis.md" ;;
    mathshim) $PY $CAP/tools/bitcompare.py harness/golden/math_shim/math_shim.json "$f" --json $CAP/reports/mathshim_bitcompare.json | tail -1 ;;
    aero)     $PY harness/compare/compare.py harness/golden/aero_forces/aero_forces.json "$f" --report $CAP/reports/aero_compare.json | tail -1
              $PY $CAP/tools/bitcompare.py harness/golden/aero_forces/aero_forces.json "$f" --tables drag_curve_value,cube_inputs,cube_weighted,cube_drag --json $CAP/reports/aero_bitcompare.json | tail -1 ;;
    c2)       $PY $CAP/tools/random_model.py --md $CAP/reports/C2_analysis.md | sed -n 3,12p
              $PY $CAP/tools/random_vec_fit.py --md $CAP/reports/C2_vector_fit.md | tail -6 ;;
    c3)       $PY $CAP/tools/c3_check.py --md $CAP/reports/C3_check.md > /dev/null && echo "wrote reports/C3_check.md" ;;
    qdextern) $PY -c "import json,sys;d=json.load(open(sys.argv[1]));print([(r[0],r[1]) for r in d['tables'][0]['rows']])" "$f" ;;
  esac
done
