# Copies the KSP input files the harness reads (read-only source: decompiled\KSP-dev) into harness\inputs and
# the game/Unity assemblies into harness\lib. Run once; run.ps1 calls it only when inputs are missing or -Resync.
param([string]$KspDev = 'C:\Projects\KSP-UE5\decompiled\KSP-dev',
      [string]$BuiltDll = 'C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\GameData\Assembly-CSharp-cleaned',
      [string]$Backup = 'C:\Projects\KSP-UE5\decompiled\backup',
      [switch]$Libs)
$ErrorActionPreference = 'Stop'
$h = $PSScriptRoot
$in = Join-Path $h 'inputs'
New-Item -ItemType Directory -Force $in | Out-Null
function RC($src, $dst, $files) {
    robocopy $src $dst $files /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy $src failed ($LASTEXITCODE)" }
}
RC "$KspDev\GameData\Squad\Parts" "$in\GameData\Squad\Parts" '*.cfg'
RC "$KspDev\GameData\Squad\Resources" "$in\GameData\Squad\Resources" '*.cfg'
RC "$KspDev\Ships" "$in\Ships" '*.craft'
Copy-Item -Force "$KspDev\Physics.cfg", "$KspDev\PartDatabase.cfg" $in
if ($Libs) {
    $lib = Join-Path $h 'lib'
    New-Item -ItemType Directory -Force $lib | Out-Null
    $m = "$KspDev\KSP_x64_Data\Managed"
    Get-ChildItem $m -Filter 'UnityEngine*.dll' | Copy-Item -Destination $lib -Force
    Get-ChildItem $m -Filter 'Unity.*.dll' | Copy-Item -Destination $lib -Force
    foreach ($f in 'Assembly-CSharp-firstpass.dll','KSPAssets.dll','KSPTrackIR.dll','Ionic.Zip.dll','Mono.Cecil.dll') { Copy-Item -Force "$m\$f" $lib }
    # a COPY of the rebuilt main assembly (another agent may rebuild the original concurrently)
    Copy-Item -Force "$BuiltDll\Assembly-CSharp.dll", "$BuiltDll\Assembly-CSharp.pdb" $lib
    Copy-Item -Force "$Backup\Assembly-CSharp-cleaned.dll" "$lib\Assembly-CSharp.de4dot-original.dll"
}
$n = (Get-ChildItem -Recurse -File $in).Count
Write-Host "inputs: $n files in $in"
