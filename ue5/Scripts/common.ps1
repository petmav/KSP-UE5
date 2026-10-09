# Shared paths and a timing logger for the ue5 helper scripts.
$Global:UeRoot   = 'C:\Program Files\Epic Games\UE_5.8'
$Global:Ubt      = Join-Path $UeRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe'
$Global:BuildBat = Join-Path $UeRoot 'Engine\Build\BatchFiles\Build.bat'
$Global:Editor   = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$Global:EditorCmd = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Global:ProjRoot = Split-Path -Parent $PSScriptRoot
$Global:Uproject = Join-Path $ProjRoot 'KspUe5.uproject'
$Global:TimingLog = Join-Path $ProjRoot 'timings.log'

function Write-Timing([string]$What, [double]$Seconds, [string]$Extra = '') {
    $line = '{0:yyyy-MM-dd HH:mm:ss}  {1,-44} {2,9:N1} s  {3}' -f (Get-Date), $What, $Seconds, $Extra
    Add-Content -Path $TimingLog -Value $line -Encoding utf8
    Write-Host "TIMING $line"
}
