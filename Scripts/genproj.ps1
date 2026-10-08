# Generates the Visual Studio solution (KspUe5.sln) via UnrealBuildTool -projectfiles.
. "$PSScriptRoot\common.ps1"
$sw = [Diagnostics.Stopwatch]::StartNew()
& $Ubt -projectfiles "-project=$Uproject" -game -rocket -progress
$rc = $LASTEXITCODE
Write-Timing 'generate project files' $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
