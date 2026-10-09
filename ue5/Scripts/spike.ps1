# Runs the phase-0 scale spike in -game mode (uncooked, editor binaries), 1280x720 windowed, then parses the log.
param([string]$Tag = 'run', [string]$Variants = '', [switch]$Quick, [int]$Stress = 1, [string]$Extra = '', [int]$TimeoutMin = 90)
. "$PSScriptRoot\common.ps1"
$logDir = Join-Path $ProjRoot 'spike\logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir "$Tag.log"
$a = @($Uproject, '/Game/Maps/ScaleSpike', '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoSound', '-unattended', '-nosplash',
       "-abslog=$log", "-SpikeTag=$Tag", "-SpikeStress=$Stress")
if ($Variants) { $a += "-SpikeVariants=$Variants" }
if ($Quick) { $a += '-SpikeQuick' }
if ($Extra) { $a += $Extra.Split(' ') }
$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath $Editor -ArgumentList $a -PassThru
if (-not $p.WaitForExit($TimeoutMin * 60 * 1000)) { Stop-Process -Id $p.Id -Force; Write-Host "SPIKE TIMEOUT after $TimeoutMin min" }
$secs = $sw.Elapsed.TotalSeconds
Select-String -Path $log -Pattern 'SPIKE_(INIT|WARMUP_DONE|STEP_DONE|DONE|WARN|KSPSCENE)|Fatal error|Unhandled Exception' | ForEach-Object { $_.Line } | Select-Object -Last 80
Write-Timing "spike run $Tag" $secs "exit=$($p.ExitCode)"
