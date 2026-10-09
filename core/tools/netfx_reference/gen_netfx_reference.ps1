<#
.SYNOPSIS
  Generates tests/data/netfx_reference.tsv: reference results of the .NET Framework 4.8 runtime (the golden harness's
  oracle runtime) for number parsing, number formatting and System.Math trig/rounding, for kspcore's unit tests.

.NOTES
  Must run on Windows PowerShell 5.1 (CLR 4.0.30319 = .NET Framework 4.x). Deterministic (fixed seed).
  Regenerate: powershell -NoProfile -ExecutionPolicy Bypass -File core\tools\netfx_reference\gen_netfx_reference.ps1
  Line formats (tab separated, bits as 16/8 hex digits):
    PD <input> <ok 0/1> <double bits>        double.TryParse(s, Float|AllowThousands, invariant)
    PF <input> <ok 0/1> <float bits>         float.TryParse(...)
    PI <input> <ok 0/1> <int32 decimal>      int.TryParse(s)
    FD <double bits> <G17> <G15/default G>   double.ToString("G17"/"G", invariant)
    FF <float bits> <G9> <G/default>         float.ToString("G9"/"G", invariant)
    MS|MC|MT|MCEIL|MROUND <x bits> <result bits>   Math.Sin/Cos/Tan/Ceiling/Round
#>
param([string] $Out = (Join-Path $PSScriptRoot '..\..\tests\data\netfx_reference.tsv'))
$ErrorActionPreference = 'Stop'
if ([Environment]::Version.Major -ne 4) { throw "needs .NET Framework 4.x (Windows PowerShell 5.1)" }

$inv = [Globalization.CultureInfo]::InvariantCulture
# invariant culture: same separators as en-US and Mono's "Infinity" / "-Infinity" / "NaN" symbols (current Windows en-US uses U+221E)
$style = [Globalization.NumberStyles]::Float -bor [Globalization.NumberStyles]::AllowThousands
$rng = New-Object System.Random 20261009
$sb = New-Object System.Text.StringBuilder

function DBits([double] $d) { '{0:X16}' -f [BitConverter]::DoubleToInt64Bits($d) }
function FBits([single] $f) { '{0:X8}' -f [BitConverter]::ToInt32([BitConverter]::GetBytes($f), 0) }
function RandDigits([int] $n) { $s = ''; for ($i = 0; $i -lt $n; $i++) { $s += [char](48 + $rng.Next(10)) }; $s }

# ---- parsing inputs
$inputs = New-Object System.Collections.Generic.List[string]
foreach ($s in @('0', '-0', '0.1', '0.85', '1.19', '2.83', '0.6960422', '1E-05', '-0.0', '1.0000001', '101.325', '3531600000000',
        '13599840256', '6.67408E-11', '9.80665', '1.7976931348623157E+308', '1.7976931348623159E+308', '4.94065645841247E-324',
        '2.4703282292062328E-324', '2.4703282292062327E-324', '1E-320', '2.2250738585072014E-308', '1,000', ' 12 ', '1e', '.5',
        '5.', '+.5e+2', '-', '', ' ', 'Infinity', '-Infinity', 'NaN', '1e400', '3.4028235E+38', '3.4028236E+38', '1e39',
        '0.30000000000000004', '123456789012345678901234567890', '0.000000000000000000000000000001234567890123456789')) {
    $inputs.Add($s)
}
for ($i = 0; $i -lt 6000; $i++) {
    $nd = 1 + $rng.Next(20)
    $d = RandDigits $nd
    $pos = $rng.Next($nd + 1)
    $s = $d.Substring(0, $pos) + '.' + $d.Substring($pos)
    if ($s.StartsWith('.')) { $s = '0' + $s }
    if ($rng.Next(3) -eq 0) { $s += 'E' + ($rng.Next(-330, 311)) }
    elseif ($rng.Next(4) -eq 0) { $s += 'e' + ($rng.Next(-40, 40)) }
    if ($rng.Next(2) -eq 0) { $s = '-' + $s }
    $inputs.Add($s)
}
foreach ($s in $inputs) {
    $d = 0.0; $okd = [double]::TryParse($s, $style, $inv, [ref] $d)
    $f = [single]0; $okf = [single]::TryParse($s, $style, $inv, [ref] $f)
    [void]$sb.AppendLine("PD`t$s`t$([int]$okd)`t$(DBits $d)")
    [void]$sb.AppendLine("PF`t$s`t$([int]$okf)`t$(FBits $f)")
}
foreach ($s in @('0', '42', ' 42 ', '-2147483648', '2147483648', '2147483647', '4.0', '1,000', '1e3', '007', '+5', '- 5', '')) {
    $n = 0; $ok = [int]::TryParse($s, [ref] $n)
    [void]$sb.AppendLine("PI`t$s`t$([int]$ok)`t$n")
}

# ---- formatting
for ($i = 0; $i -lt 4000; $i++) {
    $bits = [long]0
    $buf = New-Object byte[] 8
    $rng.NextBytes($buf)
    $d = [BitConverter]::ToDouble($buf, 0)
    if ([double]::IsNaN($d) -or [double]::IsInfinity($d)) { continue }
    if ($i % 2 -eq 0) { $d = ($rng.NextDouble() - 0.5) * [Math]::Pow(10, $rng.Next(-12, 22)) }
    [void]$sb.AppendLine("FD`t$(DBits $d)`t$($d.ToString('G17', $inv))`t$($d.ToString($inv))")
    $f = [single]$d
    if ([single]::IsInfinity($f)) { continue }
    [void]$sb.AppendLine("FF`t$(FBits $f)`t$($f.ToString('G9', $inv))`t$($f.ToString($inv))")
}

# ---- trig / rounding
$xs = New-Object System.Collections.Generic.List[double]
for ($i = 0; $i -lt 1500; $i++) { $xs.Add(($rng.NextDouble() - 0.5) * 20) }
for ($i = 0; $i -lt 800; $i++) { $xs.Add(($rng.NextDouble() - 0.5) * 2e6) }
for ($i = 0; $i -lt 300; $i++) { $xs.Add(($rng.NextDouble() - 0.5) * 2e10) }
for ($k = -40; $k -le 40; $k++) {
    $b = [Math]::PI / 2 * $k
    $bits = [BitConverter]::DoubleToInt64Bits($b)
    foreach ($o in -2..2) { $xs.Add([BitConverter]::Int64BitsToDouble($bits + $o)) }
}
foreach ($x in $xs) {
    [void]$sb.AppendLine("MS`t$(DBits $x)`t$(DBits ([Math]::Sin($x)))")
    [void]$sb.AppendLine("MC`t$(DBits $x)`t$(DBits ([Math]::Cos($x)))")
    [void]$sb.AppendLine("MT`t$(DBits $x)`t$(DBits ([Math]::Tan($x)))")
}
for ($i = 0; $i -lt 400; $i++) {
    $x = ($rng.NextDouble() - 0.5) * 8
    if ($i % 4 -eq 0) { $x = [Math]::Round($x * 2) / 2 }
    [void]$sb.AppendLine("MCEIL`t$(DBits $x)`t$(DBits ([Math]::Ceiling($x)))")
    [void]$sb.AppendLine("MROUND`t$(DBits $x)`t$(DBits ([Math]::Round($x)))")
}

$dir = Split-Path $Out
if (-not (Test-Path $dir)) { New-Item -ItemType Directory $dir | Out-Null }
[IO.File]::WriteAllText($Out, $sb.ToString(), (New-Object System.Text.UTF8Encoding $false))
Write-Host "wrote $Out ($((Get-Item $Out).Length) bytes)"
