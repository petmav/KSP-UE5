param(
    [Parameter(Mandatory = $true)][string]$Tag,
    [int]$TimeoutSec = 240,
    [int]$SettleSec = 15
)
# Launch KSP-dev, wait for the main menu scene in KSP.log, let it settle, then kill it and collect logs.
$root = 'C:\Projects\KSP-UE5\decompiled'
$ksp = Join-Path $root 'KSP-dev'
$logs = Join-Path $root 'logs'
$kspLog = Join-Path $ksp 'KSP.log'
$playerLog = Join-Path $logs "ksp-$Tag-player.log"
if (Test-Path $kspLog) { Remove-Item $kspLog -Force }
if (Test-Path $playerLog) { Remove-Item $playerLog -Force }

$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath (Join-Path $ksp 'KSP_x64.exe') -WorkingDirectory $ksp -PassThru `
    -ArgumentList @('-popupwindow', '-screen-fullscreen', '0', '-screen-width', '1280', '-screen-height', '720', '-logFile', "`"$playerLog`"")
"Started PID $($p.Id) at $(Get-Date -Format o)"
$menuAt = $null
while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
    if ($p.HasExited) { "Process EXITED by itself after $([int]$sw.Elapsed.TotalSeconds)s, exit code $($p.ExitCode)"; break }
    if (Test-Path $kspLog) {
        $hit = Select-String -LiteralPath $kspLog -Pattern 'Scene Change : From LOADING to MAINMENU|to MAINMENU' -SimpleMatch:$false -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hit) { $menuAt = [int]$sw.Elapsed.TotalSeconds; "MAINMENU reached after ${menuAt}s: $($hit.Line.Trim())"; break }
    }
    Start-Sleep -Seconds 3
}
if (-not $menuAt -and -not $p.HasExited) { "TIMEOUT: main menu not reached within ${TimeoutSec}s" }
if (-not $p.HasExited) { Start-Sleep -Seconds $SettleSec }
$alive = -not $p.HasExited
if ($alive) {
    $p.Refresh()
    "Process alive at kill time; WorkingSet=$([int]($p.WorkingSet64/1MB)) MB, Responding=$($p.Responding)"
    Stop-Process -Id $p.Id -Force
    "Killed PID $($p.Id) after $([int]$sw.Elapsed.TotalSeconds)s"
}
Start-Sleep -Seconds 2
if (Test-Path $kspLog) { Copy-Item $kspLog (Join-Path $logs "ksp-$Tag-KSP.log") -Force }
foreach ($f in @((Join-Path $logs "ksp-$Tag-KSP.log"), $playerLog)) {
    if (Test-Path $f) {
        $c = Get-Content -LiteralPath $f
        "--- $f : $($c.Count) lines"
        foreach ($pat in 'Exception', 'MissingMethodException', 'TypeLoadException', 'MissingFieldException', 'BadImageFormat', 'ReflectionTypeLoadException', 'Crash!!!', '\[AddonLoader\]', 'Assembly-CSharp', 'MAINMENU') {
            "   {0,-30} {1}" -f $pat, (@($c | Select-String -Pattern $pat)).Count
        }
    } else { "--- $f : MISSING" }
}
