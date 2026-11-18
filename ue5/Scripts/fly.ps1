# Starts a sandbox flight in -game mode (uncooked, editor binaries): core launches the craft from the pad and the shell shows it.
# Today the view is the debug one of WP-UE-07: collider proxies as lines and a fixed chase camera.
#   .\fly.ps1                                      # stock Kerbal X on the LaunchPad, game data from decompiled\KSP-dev
#   .\fly.ps1 -Craft 'analysis\physics\craft\PhysProf-M2.craft'
#   .\fly.ps1 -Shot spike\shots\pad.png -ShotFrame 300    # screenshot at sim frame 300, then quit
param([string]$Craft = 'harness\inputs\Ships\VAB\Kerbal X.craft', [string]$Site = 'LaunchPad', [string]$Shot = '', [int]$ShotFrame = 300,
      [string]$Extra = '', [int]$TimeoutMin = 30)
. "$PSScriptRoot\common.ps1"
$log = Join-Path $ProjRoot 'Saved\Logs\fly.log'
# the engine's empty map with the plain game mode (the project's default game mode is the scale spike's)
$a = @("`"$Uproject`"", '/Engine/Maps/Entry?game=/Script/Engine.GameModeBase', '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoSound',
       '-nosplash', "-abslog=`"$log`"", "-KspLaunch=`"$Craft`"", "-KspSite=$Site")
if ($Shot) {
    if (-not [IO.Path]::IsPathRooted($Shot)) { $Shot = Join-Path $ProjRoot $Shot }
    $a += @("-KspShot=`"$Shot`"", "-KspShotFrame=$ShotFrame", '-unattended')
}
if ($Extra) { $a += $Extra.Split(' ') }
$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath $Editor -ArgumentList $a -PassThru
if (-not $p.WaitForExit($TimeoutMin * 60 * 1000)) { Stop-Process -Id $p.Id -Force; Write-Host "FLY TIMEOUT after $TimeoutMin min" }
Select-String -Path $log -Pattern 'LogKspShell|LogKspCore: kspcore game|Fatal error|Unhandled Exception' | ForEach-Object { $_.Line } | Select-Object -Last 30
Write-Timing "fly $(Split-Path -Leaf $Craft)" $sw.Elapsed.TotalSeconds "exit=$($p.ExitCode)"
