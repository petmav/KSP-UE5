# Generates the shell's master materials (Content/Ksp/Materials) with the KspCreateMaterials commandlet.
. "$PSScriptRoot\common.ps1"
$sw = [Diagnostics.Stopwatch]::StartNew()
& $EditorCmd $Uproject -run=KspCreateMaterials -unattended -nopause -nullrhi -NoSound -stdout -FullStdOutLogOutput | Select-String -Pattern 'KspCreateMaterials|Error|error' | Select-Object -Last 30
$rc = $LASTEXITCODE
Write-Timing 'commandlet KspCreateMaterials' $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
