# Builds the harness toolchain: KspUnityShim -> AsmPatcher -> patched Assembly-CSharp copy -> KspGolden.
param([switch]$Quiet)
$ErrorActionPreference = 'Stop'
$h = $PSScriptRoot
$env:NUGET_PACKAGES = "$h\tools-cache\nuget-packages"
$env:DOTNET_CLI_HOME = "$h\tools-cache\dotnet-home"
$env:DOTNET_NOLOGO = '1'; $env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'; $env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'
$dn = (Get-Command dotnet -ErrorAction SilentlyContinue | Where-Object { (& $_.Source --list-sdks 2>$null) -match '^[89]\.' }) | Select-Object -First 1
$dn = if ($dn) { $dn.Source } else { 'C:\Projects\KSP-UE5\decompiled\tools\dotnet-sdk\dotnet.exe' }
function Step($name, [scriptblock]$b) { Write-Host "== $name"; & $b; if ($LASTEXITCODE -ne 0) { throw "$name failed (exit $LASTEXITCODE)" } }
Step 'build KspUnityShim' { & $dn build "$h\src\KspUnityShim\KspUnityShim.csproj" -c Release -nologo -v q }
Step 'build AsmPatcher' { & $dn build "$h\src\AsmPatcher\AsmPatcher.csproj" -c Release -nologo -v q }
Step 'patch Assembly-CSharp (rebuilt from decompile)' { & $dn "$h\build\AsmPatcher\AsmPatcher.dll" "$h\lib\Assembly-CSharp.dll" "$h\build\patched\Assembly-CSharp.dll" "$h\build\KspUnityShim\KspUnityShim.dll" "$h\build\patched\patch-report.txt" | Out-Null }
Step 'patch Assembly-CSharp (shipped IL, de4dot)' { & $dn "$h\build\AsmPatcher\AsmPatcher.dll" "$h\lib\Assembly-CSharp.de4dot-original.dll" "$h\build\patched-original\Assembly-CSharp.dll" "$h\build\KspUnityShim\KspUnityShim.dll" "$h\build\patched-original\patch-report.txt" | Out-Null }
Step 'build KspGolden' { & $dn build "$h\src\KspGolden\KspGolden.csproj" -c Release -nologo -v q }
# second copy of the generator bound to the shipped IL (cross-check of decompiler fidelity)
$orig = "$h\build\KspGolden-original"
if (Test-Path $orig) { Remove-Item -Recurse -Force $orig }
Copy-Item -Recurse "$h\build\KspGolden" $orig
Copy-Item -Force "$h\build\patched-original\Assembly-CSharp.dll" "$orig\Assembly-CSharp.dll"
if (Test-Path "$orig\Assembly-CSharp.pdb") { Remove-Item -Force "$orig\Assembly-CSharp.pdb" }
Write-Host "build ok (dotnet: $dn)"
