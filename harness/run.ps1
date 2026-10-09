# One-command regeneration of every golden dataset.
#   powershell -NoProfile -ExecutionPolicy Bypass -File harness\run.ps1 [-Resync] [-Extract] [-SkipCrossCheck]
# Steps:
#   1. inputs/ and lib/ (copies from decompiled\KSP-dev and the built DLL) - only if missing, or with -Resync
#   2. golden/serialized/*.serialized.json from the Unity assets (UnityPy) - only if missing, or with -Extract
#   3. build (build.ps1): KspUnityShim, AsmPatcher, patched copies of Assembly-CSharp (rebuilt + shipped IL), KspGolden
#   4. run KspGolden bound to the rebuilt-from-decompile IL -> golden/
#   5. run KspGolden bound to the shipped (de4dot) IL -> build/golden-original-il/, and compare with zero tolerance
param([switch]$Resync, [switch]$Extract, [switch]$SkipCrossCheck)
$ErrorActionPreference = 'Stop'
$h = $PSScriptRoot
Set-Location $h
$py = (Get-Command python -ErrorAction SilentlyContinue).Source

if ($Resync -or -not (Test-Path "$h\inputs\Physics.cfg") -or -not (Test-Path "$h\lib\Assembly-CSharp.dll")) {
    Write-Host '== sync inputs and lib'
    & "$h\sync-inputs.ps1" -Libs
}

if ($Extract -or -not (Test-Path "$h\golden\serialized\celestial_bodies.serialized.json")) {
    Write-Host '== extract serialized prefab values from the Unity assets'
    $venv = "$h\tools-cache\venv\Scripts\python.exe"
    if (-not (Test-Path $venv)) {
        & $py -m venv "$h\tools-cache\venv"
        & $venv -m pip install -q UnityPy TypeTreeGeneratorAPI capstone
    }
    & $venv -I "$h\tools\extract_assets.py" 'C:\Projects\KSP-UE5\decompiled\KSP-dev\KSP_x64_Data' "$h\lib" "$h\golden\serialized"
    if ($LASTEXITCODE -ne 0) { throw "asset extraction failed" }
}

Write-Host '== build'
& "$h\build.ps1"

$inSha = (Get-FileHash "$h\lib\Assembly-CSharp.dll" -Algorithm SHA256).Hash.ToLower()
$origSha = (Get-FileHash "$h\lib\Assembly-CSharp.de4dot-original.dll" -Algorithm SHA256).Hash.ToLower()

Write-Host '== generate golden (rebuilt-from-decompile IL)'
Get-ChildItem "$h\golden" -Directory | Where-Object { $_.Name -ne 'serialized' } | Remove-Item -Recurse -Force
& "$h\build\KspGolden\KspGolden.exe" --harness $h --out "$h\golden" --ksp-il 'rebuilt-from-decompile' --input-sha $inSha
if ($LASTEXITCODE -ne 0) { throw "KspGolden failed ($LASTEXITCODE)" }

if (-not $SkipCrossCheck) {
    Write-Host '== cross-check against the shipped IL (de4dot of the stock DLL), zero tolerance'
    $o = "$h\build\golden-original-il"
    if (Test-Path $o) { Remove-Item -Recurse -Force $o }
    & "$h\build\KspGolden-original\KspGolden.exe" --harness $h --out $o --ksp-il 'shipped-il-de4dot' --input-sha $origSha
    if ($LASTEXITCODE -ne 0) { throw "KspGolden (shipped IL) failed ($LASTEXITCODE)" }
    & $py -I "$h\compare\compare.py" "$h\golden" $o --report "$h\golden\CROSSCHECK-shipped-il.json" --quiet
    if ($LASTEXITCODE -ne 0) { Write-Warning 'rebuilt and shipped IL disagree - see golden\CROSSCHECK-shipped-il.json' }
}
Write-Host 'done. golden files:'
Get-Content "$h\golden\MANIFEST.json" | Select-String '"file"' | ForEach-Object { $_.Line.Trim() }
