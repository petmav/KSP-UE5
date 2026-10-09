#!/usr/bin/env bash
# One command on Linux/macOS: configure, build and test kspcore (mirror of run_tests.ps1).
#   ./run_tests.sh                     linux-rel, all tests
#   ./run_tests.sh -p linux-dbg        other preset (linux-rel, linux-dbg, linux-gcc-rel)
#   ./run_tests.sh -f                  --fresh reconfigure
#   ./run_tests.sh -t golden           only tests whose name matches (ctest -R)
#   ./run_tests.sh -b                  configure + build only
# Long forms: --preset P --fresh --filter RE --build-only
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
preset=linux-rel; fresh=0; filter=""; buildonly=0
while [ $# -gt 0 ]; do
  case "$1" in
    -p|--preset) preset="$2"; shift 2;;
    -f|--fresh|-Fresh) fresh=1; shift;;
    -t|--filter|-TestFilter) filter="$2"; shift 2;;
    -b|--build-only|-BuildOnly) buildonly=1; shift;;
    -h|--help) sed -n 2,9p "$0"; exit 0;;
    *) echo "unknown option $1" >&2; exit 2;;
  esac
done
cfg=(--preset "$preset"); [ $fresh = 1 ] && cfg+=(--fresh)
t0=$SECONDS
cmake "${cfg[@]}"
t1=$SECONDS
cmake --build --preset "$preset"
t2=$SECONDS
if [ $buildonly = 1 ]; then echo "configure $((t1-t0))s, build $((t2-t1))s"; exit 0; fi
targs=(--preset "$preset"); [ -n "$filter" ] && targs+=(-R "$filter")
rc=0; ctest "${targs[@]}" || rc=$?
echo "configure $((t1-t0))s, build $((t2-t1))s, test $((SECONDS-t2))s"
[ $rc = 0 ] || { echo "tests failed ($rc)"; exit $rc; }
echo "ALL TESTS PASSED"
