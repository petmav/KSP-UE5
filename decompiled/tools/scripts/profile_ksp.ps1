param(
    [Parameter(Mandatory = $true)][string]$Tag,
    [Parameter(Mandatory = $true)][string]$Craft,
    [string]$OutDir = 'C:\Projects\KSP-UE5\analysis\physics\raw',
    [int]$Width = 960,
    [int]$Height = 540,
    [double]$Settle = 10,
    [double]$Pad = 20,
    [double]$Flight = 30,
    [int]$TimeoutSec = 900
)
# Profiling-branch launcher (template: launch_ksp.ps1). Starts KSP-dev with the PhysProfDriver
# command-line switches, waits for the driver to quit the game, kills it on timeout, and collects
# KSP.log / Player.log into decompiled\logs and the CSVs into $OutDir.
$root = 'C:\Projects\KSP-UE5\decompiled'
$ksp = Join-Path $root 'KSP-dev'
$logs = Join-Path $root 'logs'
$kspLog = Join-Path $ksp 'KSP.log'
$playerLog = Join-Path $logs "ksp-prof-$Tag-player.log"
if (Get-Process -Name KSP_x64 -ErrorAction SilentlyContinue) { 'ERROR: KSP_x64 already running'; exit 2 }
if (Test-Path $kspLog) { Remove-Item $kspLog -Force }
if (Test-Path $playerLog) { Remove-Item $playerLog -Force }
New-Item -ItemType Directory -Force $OutDir | Out-Null
$sw = [Diagnostics.Stopwatch]::StartNew()
$argList = @('-popupwindow', '-screen-fullscreen', '0', '-screen-width', "$Width", '-screen-height', "$Height",
    '-logFile', "`"$playerLog`"",
    '-physprof-craft', "`"$Craft`"", '-physprof-out', "`"$OutDir`"", '-physprof-tag', $Tag,
    '-physprof-settle', "$Settle", '-physprof-pad', "$Pad", '-physprof-flight', "$Flight")
$p = Start-Process -FilePath (Join-Path $ksp 'KSP_x64.exe') -WorkingDirectory $ksp -PassThru -ArgumentList $argList
"Started PID $($p.Id) at $(Get-Date -Format o)"
$peakWs = 0
while (-not $p.HasExited -and $sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
    Start-Sleep -Seconds 5
    try { $p.Refresh(); if ($p.WorkingSet64 -gt $peakWs) { $peakWs = $p.WorkingSet64 } } catch {}
}
if (-not $p.HasExited) {
    "TIMEOUT after $([int]$sw.Elapsed.TotalSeconds)s: killing PID $($p.Id)"
    Stop-Process -Id $p.Id -Force
    Start-Sleep -Seconds 3
} else {
    "Exited by itself after $([int]$sw.Elapsed.TotalSeconds)s, exit code $($p.ExitCode)"
}
"Peak working set seen: $([int]($peakWs/1MB)) MB"
Start-Sleep -Seconds 2
if (Test-Path $kspLog) { Copy-Item $kspLog (Join-Path $logs "ksp-prof-$Tag-KSP.log") -Force }
if (Test-Path $playerLog) {
    $c = Get-Content -LiteralPath $playerLog
    "--- $playerLog : $($c.Count) lines"
    $c | Select-String -Pattern '\[PhysProf\]' | ForEach-Object { '  ' + $_.Line }
    foreach ($pat in 'Exception', 'NullReference', 'Crash!!!') {
        "   {0,-15} {1}" -f $pat, (@($c | Select-String -Pattern $pat)).Count
    }
}
Get-ChildItem $OutDir -Filter "$Tag*" | ForEach-Object { "  csv: $($_.Name) $($_.Length) bytes" }
if (Get-Process -Name KSP_x64 -ErrorAction SilentlyContinue) { 'WARNING: a KSP_x64 process is still running' }
