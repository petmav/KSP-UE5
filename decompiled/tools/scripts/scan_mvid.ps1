# Runtime proof of which Assembly-CSharp KSP actually loaded:
# launch KSP-dev, wait for main menu UI, scan the process memory for the MVID GUID bytes of
# (a) our compiled DLL, (b) the stock DLL, (c) the de4dot-cleaned DLL. Then kill KSP.
$ErrorActionPreference = 'Stop'
$ksp = 'C:\Projects\KSP-UE5\decompiled\KSP-dev'
$plog = 'C:\Projects\KSP-UE5\decompiled\logs\ksp-custom-dll-mvidscan-player.log'
$dlls = [ordered]@{
    'custom build'   = 'C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.dll'
    'stock original' = 'C:\Projects\KSP-UE5\decompiled\backup\Assembly-CSharp.dll'
    'de4dot cleaned' = 'C:\Projects\KSP-UE5\decompiled\backup\Assembly-CSharp-cleaned.dll'
}
$mvids = [ordered]@{}
foreach ($k in $dlls.Keys) {
    # read MVID in a child process so this process holds no lock / loaded copy
    $g = & powershell -NoProfile -Command "[Reflection.Assembly]::ReflectionOnlyLoadFrom('$($dlls[$k])').ManifestModule.ModuleVersionId.ToString()"
    $mvids[$k] = [Guid]$g.Trim()
    "{0,-15} MVID {1}" -f $k, $mvids[$k]
}

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class MemScan {
    [StructLayout(LayoutKind.Sequential)]
    struct MBI { public IntPtr BaseAddress, AllocationBase; public uint AllocationProtect; public IntPtr RegionSize; public uint State, Protect, Type; }
    [DllImport("kernel32")] static extern IntPtr OpenProcess(uint a, bool i, int pid);
    [DllImport("kernel32")] static extern int VirtualQueryEx(IntPtr h, IntPtr addr, out MBI m, uint len);
    [DllImport("kernel32")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, IntPtr size, out IntPtr read);
    [DllImport("kernel32")] static extern bool CloseHandle(IntPtr h);
    public static int[] Count(int pid, byte[][] pats) {
        var counts = new int[pats.Length];
        IntPtr h = OpenProcess(0x0410, false, pid);
        if (h == IntPtr.Zero) throw new Exception("OpenProcess failed");
        long addr = 0; MBI m;
        while (VirtualQueryEx(h, new IntPtr(addr), out m, (uint)Marshal.SizeOf(typeof(MBI))) != 0) {
            long size = m.RegionSize.ToInt64();
            bool readable = m.State == 0x1000 && (m.Protect & 0x100) == 0 && (m.Protect & 0x01) == 0 && m.Protect != 0;
            if (readable && size < (1L << 31)) {
                var buf = new byte[size]; IntPtr rd;
                if (ReadProcessMemory(h, m.BaseAddress, buf, new IntPtr(size), out rd)) {
                    int n = (int)rd.ToInt64();
                    for (int p = 0; p < pats.Length; p++) {
                        byte[] pat = pats[p]; byte f = pat[0];
                        for (int i = 0; i <= n - pat.Length; i++) {
                            if (buf[i] != f) continue;
                            int j = 1; while (j < pat.Length && buf[i + j] == pat[j]) j++;
                            if (j == pat.Length) counts[p]++;
                        }
                    }
                }
            }
            addr = m.BaseAddress.ToInt64() + size;
        }
        CloseHandle(h);
        return counts;
    }
}
'@

if (Test-Path $plog) { Remove-Item $plog -Force }
$p = Start-Process -FilePath "$ksp\KSP_x64.exe" -WorkingDirectory $ksp -PassThru `
    -ArgumentList @('-popupwindow', '-screen-fullscreen', '0', '-screen-width', '1280', '-screen-height', '720', '-logFile', "`"$plog`"")
$sw = [Diagnostics.Stopwatch]::StartNew(); $ok = $false
while ($sw.Elapsed.TotalSeconds -lt 180 -and -not $p.HasExited) {
    if ((Test-Path $plog) -and (Select-String -LiteralPath $plog -Pattern 'OnAppInitialized' -Quiet)) { $ok = $true; break }
    Start-Sleep -Seconds 3
}
"Main menu UI initialized: $ok after $([int]$sw.Elapsed.TotalSeconds)s (PID $($p.Id))"
if ($ok) {
    $pats = New-Object 'byte[][]' $mvids.Count
    $i = 0; foreach ($k in $mvids.Keys) { $pats[$i] = $mvids[$k].ToByteArray(); $i++ }
    "pattern lengths: $(($pats | ForEach-Object { ,$_ } | ForEach-Object { $_.Length }) -join ',')"
    $t = [Diagnostics.Stopwatch]::StartNew()
    $counts = [MemScan]::Count($p.Id, $pats)
    $i = 0
    foreach ($k in $mvids.Keys) { "{0,-15} MVID occurrences in KSP process memory: {1}" -f $k, $counts[$i]; $i++ }
    "scan took $([int]$t.Elapsed.TotalSeconds)s"
}
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; "killed KSP" }
