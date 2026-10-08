# Generates Content/Maps/ScaleSpike.umap with the KspCreateSpikeMap commandlet.
. "$PSScriptRoot\common.ps1"
$sw = [Diagnostics.Stopwatch]::StartNew()
& $EditorCmd $Uproject -run=KspCreateSpikeMap -unattended -nopause -nullrhi -NoSound -stdout -FullStdOutLogOutput | Select-String -Pattern 'KspCreateSpikeMap|Error|error' | Select-Object -Last 20
$rc = $LASTEXITCODE
Write-Timing 'commandlet KspCreateSpikeMap' $sw.Elapsed.TotalSeconds "rc=$rc"
exit $rc
