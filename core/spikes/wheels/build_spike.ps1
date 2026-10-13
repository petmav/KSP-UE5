<#
.SYNOPSIS
  Configure and build the wheel / landing-leg spike (core/WHEEL-SPIKE.md) in the spike build tree; optionally run it.

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File core\spikes\wheels\build_spike.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File core\spikes\wheels\build_spike.ps1 -Fresh
  powershell -NoProfile -ExecutionPolicy Bypass -File core\spikes\wheels\build_spike.ps1 -Run "bench rest"

.NOTES
  Presets (core/CMakePresets.json): spike-jolt (Jolt DOUBLE_PRECISION, build/spike-jolt) and spike-jolt-f32 (single precision,
  build/spike-jolt-f32). Jolt is the vendored copy (third_party/jolt). The binary is
  build/<preset>/spikes/wheels/wheel_spike.exe. Same toolchain lookup as core/run_tests.ps1. Honours CMAKE_BUILD_PARALLEL_LEVEL.
#>
param(
    [ValidateSet('spike-jolt', 'spike-jolt-f32')] [string] $Preset = 'spike-jolt',
    [switch] $Fresh,
    [string] $Run = ''
)
$ErrorActionPreference = 'Stop'
$core = Resolve-Path (Join-Path $PSScriptRoot '..\..')

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -products Microsoft.VisualStudio.Product.Community -version '[17.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (-not $vs) { $vs = & $vswhere -products * -version '[17.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1 }
if (-not $vs) { throw 'No Visual Studio 2022 installation with the x64 C++ tools found' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$envDump = & cmd.exe /s /c "`"$vcvars`" >nul 2>&1 && set"
foreach ($line in $envDump) {
    $i = $line.IndexOf('=')
    if ($i -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0, $i), $line.Substring($i + 1), 'Process') }
}
$vsCMakeDir = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake'
$cmake = Join-Path $vsCMakeDir 'CMake\bin\cmake.exe'
$ninjaDir = Join-Path $vsCMakeDir 'Ninja'
if (-not (Test-Path $cmake)) { $cmake = (Get-Command cmake).Source }
if (Test-Path (Join-Path $ninjaDir 'ninja.exe')) { $env:PATH = "$ninjaDir;$env:PATH" }

Push-Location $core
try {
    $cfgArgs = @('--preset', $Preset)
    if ($Fresh) { $cfgArgs += '--fresh' }
    & $cmake @cfgArgs
    if ($LASTEXITCODE -ne 0) { throw "configure failed ($LASTEXITCODE)" }
    & $cmake --build --preset $Preset --target wheel_spike
    if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }
    if ($Run) {
        $exe = Join-Path $core "build\$Preset\spikes\wheels\wheel_spike.exe"
        $runArgs = $Run -split ' '
        & $exe @runArgs
        if ($LASTEXITCODE -ne 0) { throw "wheel_spike failed ($LASTEXITCODE)" }
    }
}
finally { Pop-Location }
