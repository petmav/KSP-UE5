"""PE helper for the VS2013 CRT port (WP-00L): list exports, disassemble by RVA/export name with call/jump annotation.
Usage: python pe_tool.py <dll> exports [substr]
       python pe_tool.py <dll> dis <export-name|rva-hex> [maxbytes]
Needs capstone (harness/tools-cache/venv has it). Research tool only; not used by the build."""
import struct, sys, bisect
import capstone

def load(path):
    d = open(path, 'rb').read()
    peo = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, peo + 6)[0]
    optsz = struct.unpack_from('<H', d, peo + 20)[0]
    magic = struct.unpack_from('<H', d, peo + 24)[0]
    imagebase = struct.unpack_from('<Q', d, peo + 48)[0] if magic == 0x20b else struct.unpack_from('<I', d, peo + 52)[0]
    dd = peo + 24 + (112 if magic == 0x20b else 96)
    so = peo + 24 + optsz
    secs = []
    for i in range(nsec):
        name, vsize, va, rawsize, rawptr = struct.unpack_from('<8sIIII', d, so + i * 40)[:5]
        secs.append((va, vsize, rawptr, rawsize))
    return d, secs, imagebase, dd

def rva2off(secs, r):
    for va, vs, rp, rs in secs:
        if va <= r < va + max(vs, rs):
            return r - va + rp
    return None

def exports(d, secs, dd):
    er, es = struct.unpack_from('<II', d, dd)
    o = rva2off(secs, er)
    nfun, nnames, afun, anames, aord = struct.unpack_from('<IIIII', d, o + 20)
    funs = struct.unpack_from('<%dI' % nfun, d, rva2off(secs, afun))
    names = struct.unpack_from('<%dI' % nnames, d, rva2off(secs, anames))
    ords = struct.unpack_from('<%dH' % nnames, d, rva2off(secs, aord))
    out = {}
    for i in range(nnames):
        n = d[rva2off(secs, names[i]):].split(b'\0')[0].decode()
        out[n] = funs[ords[i]]
    return out

def main():
    path, cmd = sys.argv[1:3]
    d, secs, ib, dd = load(path)
    ex = exports(d, secs, dd)
    if cmd == 'exports':
        sub = sys.argv[3] if len(sys.argv) > 3 else ''
        for n, r in sorted(ex.items(), key=lambda x: x[1]):
            if sub in n: print('%08x %s' % (r, n))
        return
    tgt = sys.argv[3]
    maxb = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x400
    rva = ex[tgt] if tgt in ex else int(tgt, 16)
    byrva = {r: n for n, r in ex.items()}
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = False
    o = rva2off(secs, rva)
    code = d[o:o + maxb]
    for ins in md.disasm(code, ib + rva):
        r = ins.address - ib
        note = ''
        if ins.mnemonic in ('call', 'jmp') and ins.op_str.startswith('0x'):
            t = int(ins.op_str, 16) - ib
            note = '   ; ' + byrva.get(t, '%x' % t)
        print('%08x  %-7s %s%s' % (r, ins.mnemonic, ins.op_str, note))
        if ins.mnemonic == 'ret' and '-all' not in sys.argv: pass

main()
