param(
    [Parameter(Mandatory = $true)][string]$Tag,
    [string]$Items = 'all',
    [string]$InDir = 'C:\Projects\KSP-UE5\harness\captures\in',
    [string]$OutDir = 'C:\Projects\KSP-UE5\harness\captures\out',
    [int]$Width = 640,
    [int]$Height = 360,
    [int]$TimeoutSec = 600
)
# Harness capture launcher (template: launch_ksp.ps1 / profile_ksp.ps1). Needs the `profiling` branch build
# (PhysProfDriver + HarnessCapture). Starts KSP-dev windowed at low resolution with -harness-capture switches;
# the addon runs the captures at MAINMENU, writes <OutDir>\<dataset>.json and quits. Killed on timeout.
# Logs: decompiled\logs\ksp-capture-<Tag>-{player,KSP}.log
$root = 'C:\Projects\KSP-UE5\decompiled'
$ksp = Join-Path $root 'KSP-dev'
$logs = Join-Path $root 'logs'
$kspLog = Join-Path $ksp 'KSP.log'
$playerLog = Join-Path $logs "ksp-capture-$Tag-player.log"
if (Get-Process -Name KSP_x64 -ErrorAction SilentlyContinue) { 'ERROR: KSP_x64 already running'; exit 2 }
$branch = (& git -C (Join-Path $root 'KSP-Decompiled') rev-parse --abbrev-ref HEAD)
$commit = (& git -C (Join-Path $root 'KSP-Decompiled') rev-parse HEAD)
$dirty = (& git -C (Join-Path $root 'KSP-Decompiled') status --porcelain --untracked-files=no | Measure-Object).Count
"repo branch=$branch commit=$commit dirty_files=$dirty"
if ($branch -ne 'profiling') { "WARNING: branch is $branch; the capture addon only exists on 'profiling'" }
if ($dirty -gt 0) { $commit = "$commit+dirty" }
$dll = Join-Path $root 'KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.dll'
"DLL sha256 $((Get-FileHash $dll).Hash)  $((Get-Item $dll).LastWriteTime.ToString('o'))"
if (Test-Path $kspLog) { Remove-Item $kspLog -Force }
if (Test-Path $playerLog) { Remove-Item $playerLog -Force }
New-Item -ItemType Directory -Force $OutDir | Out-Null
$sw = [Diagnostics.Stopwatch]::StartNew()
$argList = @('-popupwindow', '-screen-fullscreen', '0', '-screen-width', "$Width", '-screen-height', "$Height",
    '-logFile', "`"$playerLog`"",
    '-harness-capture', $Items, '-harness-capture-in', "`"$InDir`"", '-harness-capture-out', "`"$OutDir`"",
    '-harness-capture-commit', $commit)
$p = Start-Process -FilePath (Join-Path $ksp 'KSP_x64.exe') -WorkingDirectory $ksp -PassThru -ArgumentList $argList
"Started PID $($p.Id) at $(Get-Date -Format o)"
while (-not $p.HasExited -and $sw.Elapsed.TotalSeconds -lt $TimeoutSec) { Start-Sleep -Seconds 3 }
if (-not $p.HasExited) {
    "TIMEOUT after $([int]$sw.Elapsed.TotalSeconds)s: killing PID $($p.Id)"
    Stop-Process -Id $p.Id -Force
    Start-Sleep -Seconds 3
} else {
    "Exited by itself after $([int]$sw.Elapsed.TotalSeconds)s, exit code $($p.ExitCode)"
}
Start-Sleep -Seconds 2
if (Test-Path $kspLog) { Copy-Item $kspLog (Join-Path $logs "ksp-capture-$Tag-KSP.log") -Force }
if (Test-Path $playerLog) {
    $c = Get-Content -LiteralPath $playerLog
    "--- $playerLog : $($c.Count) lines"
    $c | Select-String -Pattern '\[PhysProf\]' | ForEach-Object { '  ' + $_.Line.Substring(0, [Math]::Min(300, $_.Line.Length)) }
    foreach ($pat in 'Exception', 'NullReference', 'MissingMethod', 'Crash!!!', 'MAINMENU') {
        "   {0,-15} {1}" -f $pat, (@($c | Select-String -Pattern $pat)).Count
    }
}
Get-ChildItem $OutDir -Filter '*.json' | ForEach-Object { "  out: $($_.Name) $($_.Length) bytes $($_.LastWriteTime.ToString('o'))" }
if (Get-Process -Name KSP_x64 -ErrorAction SilentlyContinue) { 'WARNING: a KSP_x64 process is still running' } else { 'no KSP_x64 process running' }
