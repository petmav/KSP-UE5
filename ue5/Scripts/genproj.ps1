# Generates the Visual Studio solution (KspUe5.sln): UnrealBuildTool -projectfiles, run through Build.bat so the
# engine-bundled .NET 10 runtime is used (UnrealBuildTool.exe alone fails: no system-wide .NET 10).
. "$PSScriptRoot\common.ps1"
$sw = [Diagnostics.Stopwatch]::StartNew()
& $BuildBat -projectfiles "-project=$Uproject" -game -rocket -progress
$rc = $LASTEXITCODE
Write-Timing 'generate project files' $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
