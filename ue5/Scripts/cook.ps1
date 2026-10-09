# Cooks + stages + paks the Development Win64 game (no compile: build it first with build.ps1 -Target KspUe5).
# Output: ue5\Saved\StagedBuilds\Windows\KspUe5.exe
. "$PSScriptRoot\common.ps1"
$uat = Join-Path $UeRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$sw = [Diagnostics.Stopwatch]::StartNew()
& $uat BuildCookRun "-project=$Uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -stage -pak -skipbuild -nocompileeditor -utf8output -unattended
$rc = $LASTEXITCODE
Write-Timing 'cook + stage + pak (BuildCookRun, Development Win64)' $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
