<#
.SYNOPSIS
  Regenerates tests/data/wp03_reference.tsv: WP-03 reference values computed by KSP 1.12.5's own IL on .NET Framework 4.8.

.DESCRIPTION
  Copies the golden harness's runtime set (harness\build\KspGolden: the harness's patched Assembly-CSharp plus Unity's
  managed assemblies; read-only, nothing under harness\ is modified) into <core>\build\wp03_reference, compiles
  Wp03Ref.cs there with the .NET Framework C# compiler (csc.exe v4.0.30319, C# 5, x64) and runs it. The localization
  table comes from decompiled\KSP-dev\GameData\Squad\Localization\dictionary.cfg (en-us). Deterministic.

  powershell -NoProfile -ExecutionPolicy Bypass -File <core>\tools\wp03_reference\gen_wp03_reference.ps1
#>
param([string] $ProjectRoot = '')
$ErrorActionPreference = 'Stop'
$core = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $ProjectRoot) {
    $d = $core
    while ($d -and -not (Test-Path (Join-Path $d 'harness\compare\compare.py'))) { $d = Split-Path $d -Parent }
    if (-not $d) { throw 'KSP-UE5 project root (with harness\) not found above ' + $core }
    $ProjectRoot = $d
}
$harnessBin = Join-Path $ProjectRoot 'harness\build\KspGolden'
$dictionary = Join-Path $ProjectRoot 'decompiled\KSP-dev\GameData\Squad\Localization\dictionary.cfg'
foreach ($p in @($harnessBin, $dictionary)) { if (-not (Test-Path $p)) { throw "missing input: $p" } }

$out = Join-Path $core 'build\wp03_reference'
New-Item -ItemType Directory -Force $out | Out-Null
Copy-Item (Join-Path $harnessBin '*.dll') $out -Force

$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$refs = Get-ChildItem $out -Filter '*.dll' | ForEach-Object { '/reference:' + $_.FullName }
$exe = Join-Path $out 'Wp03Ref.exe'
& $csc /nologo /target:exe /platform:x64 /optimize+ /warn:0 "/out:$exe" /reference:System.Core.dll @refs (Join-Path $PSScriptRoot 'Wp03Ref.cs')
if ($LASTEXITCODE -ne 0) { throw "csc failed ($LASTEXITCODE)" }

$tsv = Join-Path $core 'tests\data\wp03_reference.tsv'
Push-Location $out
try {
    & $exe $tsv $dictionary
    if ($LASTEXITCODE -ne 0) { throw "Wp03Ref.exe failed ($LASTEXITCODE)" }
} finally { Pop-Location }
$n = (Get-Content $tsv | Measure-Object -Line).Lines
Write-Host "wrote $tsv ($n lines)"
