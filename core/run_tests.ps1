<#
.SYNOPSIS
  One command: configure, build and test kspcore (unit tests + golden gate against harness\golden).

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1 -Preset debug
  powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1 -Fresh -TestFilter golden

.NOTES
  Toolchain: MSVC from Visual Studio 2022 Community (located with vswhere), CMake and Ninja bundled with Visual Studio
  (Common7\IDE\CommonExtensions\Microsoft\CMake\{CMake\bin,Ninja}). Falls back to cmake/ninja on PATH.
#>
param(
    [ValidateSet('relwithdebinfo', 'debug')] [string] $Preset = 'relwithdebinfo',
    [switch] $Fresh,
    [string] $TestFilter = '',
    [switch] $BuildOnly,
    # CI mode: skip the developer-only executables (wp12_height_oracle, wp12_bench: no ctest test uses them) and, when ccache
    # is on PATH, use it as the compiler launcher (the PCH switches itself off then, see KSPCORE_PCH) and print its stats.
    [switch] $Ci
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# ---- locate Visual Studio 2022 Community (prefer it; fall back to any VS 2022 with the C++ tools)
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found at $vswhere" }
$vs = & $vswhere -products Microsoft.VisualStudio.Product.Community -version '[17.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (-not $vs) { $vs = & $vswhere -products * -version '[17.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1 }
if (-not $vs) { throw 'No Visual Studio 2022 installation with the x64 C++ tools found' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

# ---- import the x64 developer environment (cl.exe, link.exe, INCLUDE, LIB) into this process
$envDump = & cmd.exe /s /c "`"$vcvars`" >nul 2>&1 && set"
foreach ($line in $envDump) {
    $i = $line.IndexOf('=')
    if ($i -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0, $i), $line.Substring($i + 1), 'Process') }
}

# ---- CMake / Ninja: Visual Studio's bundled copies first, PATH second
$vsCMakeDir = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake'
$cmake = Join-Path $vsCMakeDir 'CMake\bin\cmake.exe'
$ctest = Join-Path $vsCMakeDir 'CMake\bin\ctest.exe'
$ninjaDir = Join-Path $vsCMakeDir 'Ninja'
if (-not (Test-Path $cmake)) { $cmake = (Get-Command cmake).Source; $ctest = (Get-Command ctest).Source }
if (Test-Path (Join-Path $ninjaDir 'ninja.exe')) { $env:PATH = "$ninjaDir;$env:PATH" }

Write-Host "VS:     $vs"
Write-Host "cl:     $((Get-Command cl.exe).Source)"
Write-Host "cmake:  $cmake ($((& $cmake --version | Select-Object -First 1)))"
Write-Host "ninja:  $((Get-Command ninja.exe).Source)"

$ccache = $null
if ($Ci) {
    $ccache = Get-Command ccache -ErrorAction SilentlyContinue
    if ($ccache) {
        if (-not $env:CMAKE_C_COMPILER_LAUNCHER)   { $env:CMAKE_C_COMPILER_LAUNCHER   = 'ccache' }
        if (-not $env:CMAKE_CXX_COMPILER_LAUNCHER) { $env:CMAKE_CXX_COMPILER_LAUNCHER = 'ccache' }
        Write-Host "ccache: $($ccache.Source)"
    }
}

Push-Location $root
try {
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $cfgArgs = @('--preset', $Preset)
    if ($Fresh) { $cfgArgs += '--fresh' }
    if ($Ci) { $cfgArgs += '-DKSPCORE_BUILD_DEV_TOOLS=OFF' }
    & $cmake @cfgArgs
    if ($LASTEXITCODE -ne 0) { throw "configure failed ($LASTEXITCODE)" }
    $tCfg = $sw.Elapsed.TotalSeconds

    & $cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }
    $tBuild = $sw.Elapsed.TotalSeconds - $tCfg
    if ($ccache) { & ccache -s }
    if ($BuildOnly) { Write-Host ("configure {0:N1}s, build {1:N1}s" -f $tCfg, $tBuild); return }

    $testArgs = @('--preset', $Preset)
    if ($TestFilter) { $testArgs += @('-R', $TestFilter) }
    $testArgs += @('-j', [string]([Math]::Max(2, [int]$env:NUMBER_OF_PROCESSORS)))
    & $ctest @testArgs
    $rc = $LASTEXITCODE
    $tTest = $sw.Elapsed.TotalSeconds - $tCfg - $tBuild
    Write-Host ("configure {0:N1}s, build {1:N1}s, test {2:N1}s" -f $tCfg, $tBuild, $tTest)
    if ($rc -ne 0) { throw "tests failed ($rc)" }
    Write-Host 'ALL TESTS PASSED'
}
finally { Pop-Location }
