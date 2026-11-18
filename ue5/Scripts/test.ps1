# Runs the KspUe5 automation tests headless (no GPU: -nullrhi) and prints the result lines.
param([string]$Filter = 'KspUe5', [string]$Label = 'headless automation tests')
. "$PSScriptRoot\common.ps1"
$log = Join-Path $ProjRoot 'Saved\Logs\automation.log'
$report = Join-Path $ProjRoot 'Saved\Automation'
# The ctest twin of KspUe5.Bridge.JoltStep: steps the same core scenario through the same test ABI and writes its hash to
# core\build\RelWithDebInfo\kspcore_jolt_step.txt, which the automation test compares with. Re-run it every time so the file
# always belongs to the kspcore.lib that is linked (skipped when the core was not built).
$coreBuild = Join-Path (Split-Path -Parent $ProjRoot) 'core\build\RelWithDebInfo'
$twin = Join-Path $coreBuild 'tests\capi_jolt_step.exe'
if (Test-Path $twin) { & $twin (Join-Path $coreBuild 'kspcore_jolt_step.txt') 200 | Select-Object -Last 1 }
# The ctest twin of KspUe5.Slice.HeadlessParity (ctest capi.flight_slice, same arguments as core\tests\CMakeLists.txt): flies PhysProf-M2
# through the C ABI and writes the final state hash to tests\capi_flight_slice_work\final.hash (about 15 s).
$slice = Join-Path $coreBuild 'tests\capi_flight_slice.exe'
if ((Test-Path $slice) -and ('KspUe5.Slice.HeadlessParity' -like "$Filter*")) {
    $repo = Split-Path -Parent $ProjRoot
    $work = Join-Path $coreBuild 'tests\capi_flight_slice_work'
    New-Item -ItemType Directory -Force -Path $work | Out-Null
    & $slice "$repo\core\testdata\GameData-mirror" "$repo\core\data" "$repo\core\tests\data" "$work\ksp\saves" `
        "$repo\analysis\physics\craft\PhysProf-M2.craft" "$repo\core\tests\data\c10_states.csv" "$work\final.hash" | Select-Object -Last 1
}
$sw = [Diagnostics.Stopwatch]::StartNew()
& $EditorCmd $Uproject "-ExecCmds=Automation RunTests $Filter; Quit" -unattended -nopause -nullrhi -NoSound -NoSplash `
    "-ReportExportPath=$report" "-abslog=$log" -log | Out-Null
$rc = $LASTEXITCODE
$secs = $sw.Elapsed.TotalSeconds
Select-String -Path $log -Pattern 'Test Completed|Result=|Error:|kspcore build info|max per-frame jitter|level \d+: edge|state hash|world tick|TEST COMPLETE|Tests? (Passed|Failed)' |
    ForEach-Object { $_.Line } | Select-Object -Last 40
Write-Timing $Label $secs "rc=$rc"
exit $rc
