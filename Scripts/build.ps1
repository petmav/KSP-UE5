# Builds a target from the command line. Examples:
#   .\build.ps1                         # KspUe5Editor Win64 Development
#   .\build.ps1 -Target KspUe5          # Development game
#   .\build.ps1 -Module KspCoreBridge   # one module of the editor target (plugin-only rebuild)
param([string]$Target = 'KspUe5Editor', [string]$Config = 'Development', [string]$Module = '', [string]$Label = '')
. "$PSScriptRoot\common.ps1"
$args2 = @($Target, 'Win64', $Config, "-Project=$Uproject", '-WaitMutex', '-NoHotReloadFromIDE')
if ($Module) { $args2 += "-Module=$Module" }
$sw = [Diagnostics.Stopwatch]::StartNew()
& $BuildBat @args2
$rc = $LASTEXITCODE
if (-not $Label) { $Label = "build $Target $Config" + $(if ($Module) { " -Module=$Module" } else { '' }) }
Write-Timing $Label $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
