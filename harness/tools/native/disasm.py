"""Disassemble functions of UnityPlayer.dll by RVA (x64), annotating call targets with PDB public names.
Usage: python disasm.py <pe> <publics.tsv> <rva_hex|symbol-substring> [maxbytes]
"""
import struct, sys, bisect
import capstone

def load_pe(pe):
    d = open(pe, 'rb').read()
    peo = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, peo + 6)[0]
    optsz = struct.unpack_from('<H', d, peo + 20)[0]
    imagebase = struct.unpack_from('<Q', d, peo + 24 + 24)[0]
    so = peo + 24 + optsz
    secs = []
    for i in range(nsec):
        name, vsize, va, rawsize, rawptr = struct.unpack_from('<8sIIII', d, so + i * 40)
        secs.append((va, vsize, rawptr, rawsize))
    return d, secs, imagebase

def rva2off(secs, r):
    for va, vs, rp, rs in secs:
        if va <= r < va + max(vs, rs):
            return r - va + rp
    return None

def main():
    pe, pubs, target = sys.argv[1:4]
    maxb = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x600
    d, secs, ib = load_pe(pe)
    names = {}
    rvas = []
    for line in open(pubs, encoding='utf-8'):
        r, f, n = line.rstrip('\n').split('\t', 2)
        r = int(r, 16)
        names.setdefault(r, n)
    rvas = sorted(names)
    if all(c in '0123456789abcdefABCDEF' for c in target):
        rva = int(target, 16)
    else:
        cands = [r for r in rvas if target in names[r]]
        rva = cands[0]
    # end at next public symbol
    idx = bisect.bisect_right(rvas, rva)
    end = rvas[idx] if idx < len(rvas) else rva + maxb
    size = min(end - rva, maxb)
    off = rva2off(secs, rva)
    code = d[off:off + size]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    print('; %08X %s (%d bytes)' % (rva, names.get(rva, '?'), size))
    for ins in md.disasm(code, rva):
        ann = ''
        if ins.mnemonic in ('call', 'jmp') or ins.mnemonic.startswith('j'):
            try:
                t = int(ins.op_str, 16)
                if t in names: ann = '  ; -> ' + names[t]
            except ValueError:
                pass
        if 'rip' in ins.op_str:
            # compute rip-relative target
            import re
            m = re.search(r'\[rip ([+-]) 0x([0-9a-f]+)\]', ins.op_str)
            if m:
                disp = int(m.group(2), 16) * (1 if m.group(1) == '+' else -1)
                t = ins.address + ins.size + disp
                o = rva2off(secs, t)
                extra = ''
                if o is not None:
                    if 'dword' in ins.op_str or 'ss' in ins.mnemonic:
                        extra = ' float=%r' % struct.unpack_from('<f', d, o)[0]
                    elif 'qword' in ins.op_str or 'sd' in ins.mnemonic:
                        extra = ' double=%r' % struct.unpack_from('<d', d, o)[0]
                    elif 'xmmword' in ins.op_str:
                        extra = ' xmm=%r' % (struct.unpack_from('<4f', d, o),)
                ann = '  ; [%08X]%s %s' % (t, extra, names.get(t, ''))
        print('%08X  %-8s %s%s' % (ins.address, ins.mnemonic, ins.op_str, ann))

main()
