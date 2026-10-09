# Runs the KspUe5 automation tests headless (no GPU: -nullrhi) and prints the result lines.
param([string]$Filter = 'KspUe5', [string]$Label = 'headless automation tests')
. "$PSScriptRoot\common.ps1"
$log = Join-Path $ProjRoot 'Saved\Logs\automation.log'
$report = Join-Path $ProjRoot 'Saved\Automation'
$sw = [Diagnostics.Stopwatch]::StartNew()
& $EditorCmd $Uproject "-ExecCmds=Automation RunTests $Filter; Quit" -unattended -nopause -nullrhi -NoSound -NoSplash `
    "-ReportExportPath=$report" "-abslog=$log" -log | Out-Null
$rc = $LASTEXITCODE
$secs = $sw.Elapsed.TotalSeconds
Select-String -Path $log -Pattern 'Test Completed|Result=|Error:|kspcore build info|max per-frame jitter|level \d+: edge|TEST COMPLETE|Tests? (Passed|Failed)' |
    ForEach-Object { $_.Line } | Select-Object -Last 40
Write-Timing $Label $secs "rc=$rc"
exit $rc
