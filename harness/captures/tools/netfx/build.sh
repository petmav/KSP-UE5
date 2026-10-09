#!/usr/bin/env bash
# Build LibmRef.exe (.NET Framework 4.8, x64, /optimize+) from the C7 kernel copied from the profiling branch.
# Usage: build.sh [copy]   ("copy" refreshes src/ from decompiled/KSP-Decompiled working tree; needs branch profiling checked out)
set -e
D=$(cd "$(dirname "$0")" && pwd)
R=/c/Projects/KSP-UE5/decompiled/KSP-Decompiled
mkdir -p "$D/src" "$D/bin"
if [ "$1" = "copy" ]; then
  test "$(git -C $R rev-parse --abbrev-ref HEAD)" = profiling
  cp "$R/Assembly-CSharp/PhysProf/HarnessCaptureC7.cs" "$R/Assembly-CSharp/PhysProf/HarnessCaptureJson.cs" "$D/src/"
  echo "copied from profiling @ $(git -C $R rev-parse --short HEAD) (working tree)" > "$D/src/SOURCE.txt"
fi
CSC="/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/Roslyn/csc.exe"
FW='C:\Windows\Microsoft.NET\Framework64\v4.0.30319'
cd "$D"
"$CSC" -nologo -noconfig -nostdlib+ -platform:x64 -optimize+ -langversion:7.3 -target:exe -out:bin/LibmRef.exe \
  "-r:$FW\mscorlib.dll" "-r:$FW\System.dll" "-r:$FW\System.Core.dll" \
  LibmRef.cs src/HarnessCaptureC7.cs src/HarnessCaptureJson.cs
sha256sum src/*.cs bin/LibmRef.exe
