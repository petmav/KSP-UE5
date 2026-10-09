# Builds ue5/Stub/lib/kspcore.lib (placeholder for the real core) with CMake + VS 2022, RelWithDebInfo, /MD.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Stub = Join-Path $Root 'Stub'
$Build = Join-Path $Stub 'build'
cmake -S $Stub -B $Build -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
cmake --build $Build --config RelWithDebInfo
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
Get-Item (Join-Path $Stub 'lib\kspcore.lib') | Select-Object FullName, Length, LastWriteTime
