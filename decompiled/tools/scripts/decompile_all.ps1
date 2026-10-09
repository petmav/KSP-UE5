<#
.SYNOPSIS
  Read-only ILSpy 8.2 decompilation of every remaining managed assembly shipped with KSP 1.12.5
  (KSP-dev copy) into KSP-Decompiled\<Group>\<AssemblyName>\. Not built; reference sources only.

.DESCRIPTION
  Groups (output folder under KSP-Decompiled\):
    Unity       KSP_x64_Data\Managed\UnityEngine*.dll, Unity.*.dll                          C# latest
    Mono        KSP_x64_Data\Managed\ mscorlib, System*, Mono.Security, Mono.Posix, I18N*      C# latest
    ThirdParty  KSP_x64_Data\Managed\ everything managed that is not Squad / Unity / Mono      C# 7.3
    Launcher    every managed DLL in Launcher_Data\Managed\ (flat)       Unity/Mono: latest, others: C# 7.3
    Expansions  GameData\SquadExpansion\**\Plugins\*.dll + GameData\Squad\Plugins\*.dll except
                KSPSteamCtrlr (already in KSP-Decompiled\KSPSteamCtrlr)                       C# 7.3
  Squad assemblies already decompiled earlier (Assembly-CSharp, Assembly-CSharp-firstpass, KSPAssets,
  KSPAssets.XmlSerializers, KSPTrackIR, KSPSteamCtrlr) are excluded from the game-Managed groups.

  Per group: de4dot -d on all files (logged); any assembly with a *known* obfuscator is first cleaned
  with `de4dot --dont-rename` into tools\scratch\de4dot-<group>\ and the cleaned copy is decompiled.
  Then tools\IlspyExport (ICSharpCode.Decompiler 8.2.0.7535, legacy csproj, nested namespace dirs)
  runs once per assembly with a wall-clock limit (-TimeoutMin); on timeout/crash the partial output is
  removed and the assembly is recorded as SKIPPED/FAILED. Counts: *.cs files and newline count
  (same semantics as `wc -l`).

  Outputs: logs\decompile-all-<group>.log (full exporter output), logs\decompile-all-<group>.csv
  (one row per assembly). -Inventory writes logs\decompile-all-inventory.csv instead: every .dll/.exe
  in KSP-dev with managed/native, origin, sha256, version and de4dot detection.

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\decompile_all.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\decompile_all.ps1 -Group Mono -Force
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\decompile_all.ps1 -Inventory
#>
param(
    [string[]]$Group = @('Unity', 'Mono', 'ThirdParty', 'Launcher', 'Expansions'),   # also accepts 'A,B' (powershell -File)
    [int]$TimeoutMin = 20,
    [switch]$Force,
    [switch]$Inventory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2
$Group = @($Group | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
foreach ($g in $Group) { if (@('Unity', 'Mono', 'ThirdParty', 'Launcher', 'Expansions') -notcontains $g) { throw "unknown group '$g'" } }

$root = 'C:\Projects\KSP-UE5\decompiled'
$ksp = Join-Path $root 'KSP-dev'
$gameManaged = Join-Path $ksp 'KSP_x64_Data\Managed'
$launcherManaged = Join-Path $ksp 'Launcher_Data\Managed'
$outRoot = Join-Path $root 'KSP-Decompiled'
$logs = Join-Path $root 'logs'
$dotnet = Join-Path $root 'tools\dotnet-sdk\dotnet.exe'
$exporter = Join-Path $root 'tools\IlspyExport\out\IlspyExport.dll'
$de4dot = Join-Path $root 'tools\de4dot\net48\de4dot.exe'
$stockAsm = Join-Path $root 'backup\Assembly-CSharp.dll'   # stock (obfuscated) original; Managed\ copy is a symlink to our build

$SquadNames = @('Assembly-CSharp', 'Assembly-CSharp-firstpass', 'KSPAssets', 'KSPAssets.XmlSerializers', 'KSPTrackIR', 'KSPSteamCtrlr')

Add-Type -TypeDefinition @'
using System.IO;
public static class CsCount {
    // returns { .cs file count, newline count over those files }  (newline count == `wc -l`)
    public static long[] Count(string dir) {
        long files = 0, lines = 0;
        foreach (var f in Directory.EnumerateFiles(dir, "*.cs", SearchOption.AllDirectories)) {
            files++;
            foreach (byte b in File.ReadAllBytes(f)) if (b == 10) lines++;
        }
        return new long[] { files, lines };
    }
}
'@

function Get-Origin([string]$name) {
    if ($SquadNames -contains $name) { return 'Squad' }
    if ($name -like 'UnityEngine*' -or $name -like 'Unity.*') { return 'Unity' }
    if ($name -eq 'mscorlib' -or $name -eq 'netstandard' -or $name -eq 'System' -or $name -like 'System.*' -or
        $name -eq 'Mono.Security' -or $name -eq 'Mono.Posix' -or $name -eq 'I18N' -or $name -like 'I18N.*') { return 'Mono' }
    return 'third-party'
}

function Test-Managed([string]$path) {
    try { [void][Reflection.AssemblyName]::GetAssemblyName($path); return $true }
    catch { return $false }
}

function Get-AsmVersion([string]$path) {
    try { return [Reflection.AssemblyName]::GetAssemblyName($path).Version.ToString() } catch { return '' }
}

function Get-RelPath([string]$path, [string]$base) {
    $p = [IO.Path]::GetFullPath($path); $b = [IO.Path]::GetFullPath($base).TrimEnd('\') + '\'
    if ($p.StartsWith($b, [StringComparison]::OrdinalIgnoreCase)) { return $p.Substring($b.Length) }
    return $p
}

# de4dot -d on a set of files -> hashtable fullpath(lower) -> detected obfuscator string
function Invoke-Detect([string[]]$files, [string]$logFile) {
    $map = @{}
    if (-not $files -or $files.Count -eq 0) { return $map }
    $out = & $de4dot -d @files 2>&1 | ForEach-Object { "$_" }
    if ($logFile) { Add-Content -LiteralPath $logFile -Value (@('### de4dot -d') + $out) }
    foreach ($line in $out) {
        if ($line -match '^Detected (.+?) \((.+)\)\s*$') { $map[$Matches[2].ToLowerInvariant()] = $Matches[1] }
    }
    return $map
}

function Get-GroupFiles([string]$g) {
    $all = @()
    switch ($g) {
        'Unity' { $all = @(Get-ChildItem -LiteralPath $gameManaged -Filter *.dll | Where-Object { (Get-Origin $_.BaseName) -eq 'Unity' }) }
        'Mono' { $all = @(Get-ChildItem -LiteralPath $gameManaged -Filter *.dll | Where-Object { (Get-Origin $_.BaseName) -eq 'Mono' }) }
        'ThirdParty' { $all = @(Get-ChildItem -LiteralPath $gameManaged -Filter *.dll | Where-Object { (Get-Origin $_.BaseName) -eq 'third-party' }) }
        'Launcher' { $all = @(Get-ChildItem -LiteralPath $launcherManaged -Filter *.dll) }
        'Expansions' {
            $exp = Join-Path $ksp 'GameData\SquadExpansion'
            if (Test-Path -LiteralPath $exp) { $all += @(Get-ChildItem -LiteralPath $exp -Recurse -Filter *.dll | Where-Object { $_.FullName -match '\\Plugins\\' }) }
            $sp = Join-Path $ksp 'GameData\Squad\Plugins'
            if (Test-Path -LiteralPath $sp) { $all += @(Get-ChildItem -LiteralPath $sp -Filter *.dll | Where-Object { $_.BaseName -ne 'KSPSteamCtrlr' }) }
        }
    }
    return @($all | Where-Object { Test-Managed $_.FullName } | Sort-Object Name)
}

function Get-LangFor([string]$g, [string]$name) {
    $o = Get-Origin $name
    if ($o -eq 'Unity' -or $o -eq 'Mono') { return 'latest' }
    return '7.3'
}

function Invoke-Group([string]$g) {
    $log = Join-Path $logs "decompile-all-$g.log"
    $csv = Join-Path $logs "decompile-all-$g.csv"
    Set-Content -LiteralPath $log -Value "decompile_all.ps1 group=$g started $(Get-Date -Format o) TimeoutMin=$TimeoutMin Force=$Force"
    $files = @(Get-GroupFiles $g)
    Add-Content -LiteralPath $log -Value "assemblies: $($files.Count)"
    "[$g] $($files.Count) managed assemblies"
    $rows = New-Object System.Collections.Generic.List[object]
    if ($files.Count -eq 0) {
        Add-Content -LiteralPath $log -Value 'nothing to do (no managed DLLs found for this group)'
        Set-Content -LiteralPath $csv -Value 'group,assembly,origin,source,sha256,version,obfuscator,deobfuscated,lang,status,exitCode,seconds,peakMB,unresolvedRefs,files,lines,outDir'
        return
    }
    $det = Invoke-Detect -files @($files | ForEach-Object { $_.FullName }) -logFile $log
    $refDir = if ($g -eq 'Launcher') { $launcherManaged } else { $gameManaged }
    $scratch = Join-Path $root "tools\scratch\de4dot-$g"

    foreach ($f in $files) {
        $name = $f.BaseName
        $outDir = Join-Path $outRoot "$g\$name"
        $obf = $det[$f.FullName.ToLowerInvariant()]
        if (-not $obf) { $obf = '?' }
        $isObf = ($obf -ne 'Unknown Obfuscator' -and $obf -ne '?')
        $lang = Get-LangFor $g $name
        $row = [ordered]@{
            group = $g; assembly = $name; origin = (Get-Origin $name); source = (Get-RelPath $f.FullName $ksp)
            sha256 = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash; version = (Get-AsmVersion $f.FullName)
            obfuscator = $(if ($isObf) { $obf } else { 'none' }); deobfuscated = 'n'; lang = $lang
            status = ''; exitCode = ''; seconds = ''; peakMB = ''; unresolvedRefs = ''; files = ''; lines = ''; outDir = (Get-RelPath $outDir $outRoot)
        }
        Add-Content -LiteralPath $log -Value "`r`n##### $name  ($($f.FullName))  lang=$lang obfuscator=$obf"

        $exists = (Test-Path -LiteralPath $outDir) -and @(Get-ChildItem -LiteralPath $outDir -Force).Count -gt 0
        if ($exists -and -not $Force) {
            Add-Content -LiteralPath $log -Value 'output exists, skipped (use -Force to redo)'
            $row.status = 'exists'
        } else {
            if (Test-Path -LiteralPath $outDir) { Remove-Item -LiteralPath $outDir -Recurse -Force }
            $srcAsm = $f.FullName; $proj = $name
            if ($isObf) {
                New-Item -ItemType Directory -Force -Path $scratch | Out-Null
                $clean = Join-Path $scratch "$name-cleaned.dll"
                if (Test-Path -LiteralPath $clean) { Remove-Item -LiteralPath $clean -Force }
                $d = & $de4dot --dont-rename $f.FullName -o $clean 2>&1 | ForEach-Object { "$_" }
                Add-Content -LiteralPath $log -Value (@('### de4dot --dont-rename') + $d)
                if (-not (Test-Path -LiteralPath $clean)) { throw "de4dot did not produce $clean" }
                $srcAsm = $clean; $proj = "$name-cleaned"; $row.deobfuscated = 'y'
            }
            $argList = @($exporter, $srcAsm, $outDir, $proj, '--lang', $lang, $refDir) | ForEach-Object { '"' + $_ + '"' }
            $so = [IO.Path]::GetTempFileName(); $se = [IO.Path]::GetTempFileName()
            $sw = [Diagnostics.Stopwatch]::StartNew()
            $p = Start-Process -FilePath $dotnet -ArgumentList $argList -NoNewWindow -PassThru -RedirectStandardOutput $so -RedirectStandardError $se
            $null = $p.Handle   # PS 5.1: needed so ExitCode is available after exit
            $peak = 0L; $timedOut = $false
            while (-not $p.HasExited) {
                try { $p.Refresh(); if ($p.PeakWorkingSet64 -gt $peak) { $peak = $p.PeakWorkingSet64 } } catch { }
                if ($sw.Elapsed.TotalMinutes -gt $TimeoutMin) { $timedOut = $true; try { $p.Kill() } catch { }; break }
                Start-Sleep -Milliseconds 200
            }
            $p.WaitForExit()
            $sw.Stop()
            $stdout = Get-Content -LiteralPath $so; $stderr = Get-Content -LiteralPath $se
            Remove-Item -LiteralPath $so, $se -Force
            Add-Content -LiteralPath $log -Value $stdout
            if ($stderr) { Add-Content -LiteralPath $log -Value (@('### stderr') + $stderr) }
            $row.seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1)
            $row.peakMB = [int]($peak / 1MB)
            $row.exitCode = $(if ($timedOut) { 'killed' } else { $p.ExitCode })
            $row.unresolvedRefs = @($stdout | Where-Object { $_ -match '-> UNRESOLVED' }).Count
            if ($timedOut) {
                $row.status = "SKIPPED-timeout-${TimeoutMin}min"
            } elseif ($p.ExitCode -ne 0) {
                $row.status = 'FAILED'
            } else {
                $row.status = 'ok'
            }
            if ($row.status -ne 'ok' -and (Test-Path -LiteralPath $outDir)) {
                Remove-Item -LiteralPath $outDir -Recurse -Force
                Add-Content -LiteralPath $log -Value "partial output removed: $outDir"
            }
            Add-Content -LiteralPath $log -Value "status=$($row.status) exit=$($row.exitCode) seconds=$($row.seconds) peakMB=$($row.peakMB)"
        }
        if (Test-Path -LiteralPath $outDir) {
            $c = [CsCount]::Count($outDir); $row.files = $c[0]; $row.lines = $c[1]
        }
        "[{0}] {1,-55} {2,-8} {3,6} files {4,9} lines {5,7}s {6,6}MB" -f $g, $name, $row.status, $row.files, $row.lines, $row.seconds, $row.peakMB
        $rows.Add([pscustomobject]$row)
    }
    $rows | Export-Csv -LiteralPath $csv -NoTypeInformation -Encoding UTF8
    $ok = @($rows | Where-Object { $_.status -eq 'ok' -or $_.status -eq 'exists' })
    $tf = ($ok | Measure-Object -Property files -Sum).Sum; $tl = ($ok | Measure-Object -Property lines -Sum).Sum
    $summary = "[$g] done: $($ok.Count)/$($rows.Count) assemblies decompiled, $tf .cs files, $tl lines; not ok: " +
        ((@($rows | Where-Object { $_.status -ne 'ok' -and $_.status -ne 'exists' }) | ForEach-Object { "$($_.assembly)=$($_.status)" }) -join ', ')
    Add-Content -LiteralPath $log -Value "`r`n$summary`r`nfinished $(Get-Date -Format o)"
    $summary
}

function Invoke-Inventory {
    $csv = Join-Path $logs 'decompile-all-inventory.csv'
    $log = Join-Path $logs 'decompile-all-inventory.log'
    Set-Content -LiteralPath $log -Value "inventory started $(Get-Date -Format o)"
    $items = @(Get-ChildItem -LiteralPath $ksp -Recurse -File | Where-Object { $_.Extension -eq '.dll' -or $_.Extension -eq '.exe' } | Sort-Object FullName)
    $gameHashes = @{}
    foreach ($f in Get-ChildItem -LiteralPath $gameManaged -Filter *.dll) { $gameHashes[$f.Name] = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash }
    $managedPaths = New-Object System.Collections.Generic.List[string]
    $info = @()
    foreach ($f in $items) {
        $rel = Get-RelPath $f.FullName $ksp
        $isLink = [bool]($f.Attributes -band [IO.FileAttributes]::ReparsePoint)
        $probe = $f.FullName
        $note = ''
        if ($isLink -and $f.Name -eq 'Assembly-CSharp.dll' -and $f.DirectoryName -eq $gameManaged) {
            $probe = $stockAsm; $note = 'symlink to custom build; stock original probed at backup\Assembly-CSharp.dll'
        }
        $managed = Test-Managed $probe
        if ($managed) { $managedPaths.Add($probe) }
        $info += [pscustomobject]@{ File = $f; Rel = $rel; Probe = $probe; Managed = $managed; Note = $note }
    }
    $det = Invoke-Detect -files $managedPaths.ToArray() -logFile $log
    $rows = foreach ($i in $info) {
        $name = [IO.Path]::GetFileNameWithoutExtension($i.File.Name)
        $hash = (Get-FileHash -LiteralPath $i.Probe -Algorithm SHA256).Hash
        $dup = ''
        if ($i.Rel -like 'Launcher_Data\Managed\*' -and $gameHashes.ContainsKey($i.File.Name) -and $gameHashes[$i.File.Name] -eq $hash) { $dup = "identical to KSP_x64_Data\Managed\$($i.File.Name)" }
        $obf = ''
        if ($i.Managed) { $o = $det[$i.Probe.ToLowerInvariant()]; $obf = $(if (-not $o) { '?' } elseif ($o -eq 'Unknown Obfuscator') { 'none' } else { $o }) }
        [pscustomobject][ordered]@{
            path = $i.Rel; name = $name; ext = $i.File.Extension.ToLowerInvariant(); managed = $(if ($i.Managed) { 'y' } else { 'n' })
            origin = $(if ($i.Managed) { Get-Origin $name } else { 'native' }); version = $(if ($i.Managed) { Get-AsmVersion $i.Probe } else { '' })
            bytes = (Get-Item -LiteralPath $i.Probe).Length; sha256 = $hash; obfuscator = $obf; duplicateOf = $dup; note = $i.Note
        }
    }
    $rows | Export-Csv -LiteralPath $csv -NoTypeInformation -Encoding UTF8
    "inventory: $(@($rows).Count) files ($(@($rows | Where-Object managed -eq 'y').Count) managed, $(@($rows | Where-Object managed -eq 'n').Count) native) -> $csv"
}

if ($Inventory) { Invoke-Inventory; return }
foreach ($g in $Group) { Invoke-Group $g }
