#!/usr/bin/env bash
# Usage: build_opt.sh <tag> [Build|Rebuild]  -> logs/build-<tag>.{log,errors.log,warnings.log}
# Same as build.sh but compiles with -p:Optimize=true (csc /optimize+), like the shipped game DLL.
# Used for the profiling-branch builds (PROFILE.md "Build and machine") and the harness captures.
MSB="/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/amd64/MSBuild.exe"
L=/c/Projects/KSP-UE5/decompiled/logs
WL='C:\Projects\KSP-UE5\decompiled\logs'
T=${2:-Build}
cd /c/Projects/KSP-UE5/decompiled/KSP-Decompiled
"$MSB" Assembly-CSharp-cleaned.sln -t:$T -p:Configuration=Debug -p:Optimize=true -nologo -v:quiet \
  "-flp:LogFile=$WL\build-$1.log;Verbosity=normal" \
  "-flp2:LogFile=$WL\build-$1.errors.log;ErrorsOnly" \
  "-flp3:LogFile=$WL\build-$1.warnings.log;WarningsOnly" > $L/build-$1.console.txt 2>&1
rc=$?
echo "msbuild exit $rc"
echo "errors:   $(grep -cE 'error (CS|MSB)' $L/build-$1.errors.log)"
echo "warnings: $(wc -l < $L/build-$1.warnings.log)"
echo "branch:   $(git rev-parse --abbrev-ref HEAD) $(git rev-parse --short HEAD)"
sha256sum GameData/Assembly-CSharp-cleaned/Assembly-CSharp.dll
