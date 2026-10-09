"""Minimal MSF 7.0 / PDB reader: dumps public symbols (S_PUB32) with RVAs.

Usage: python -I pdb_publics.py <pdb> <pe> <out.tsv>
Only used to locate Unity native AnimationCurve code in UnityPlayer.dll for the
golden harness; no third-party dependencies.
"""
import struct, sys

def read_msf(path):
    f = open(path, 'rb')
    sb = f.read(56)
    assert sb[:24] == b'Microsoft C/C++ MSF 7.00', 'not an MSF 7.0 file'
    bs, fbm, nblocks, ndirbytes, _unk, bmaddr = struct.unpack_from('<6I', sb, 32)
    def block(i):
        f.seek(i * bs); return f.read(bs)
    ndirblocks = (ndirbytes + bs - 1) // bs
    bm = block(bmaddr)
    dirblocks = struct.unpack_from('<%dI' % ndirblocks, bm, 0)
    d = b''.join(block(b) for b in dirblocks)[:ndirbytes]
    nstreams = struct.unpack_from('<I', d, 0)[0]
    sizes = struct.unpack_from('<%dI' % nstreams, d, 4)
    off = 4 + 4 * nstreams
    streams = []
    for s in sizes:
        if s == 0xFFFFFFFF: s = 0
        nb = (s + bs - 1) // bs
        blks = struct.unpack_from('<%dI' % nb, d, off); off += 4 * nb
        streams.append((s, blks))
    def stream(i):
        s, blks = streams[i]
        return b''.join(block(b) for b in blks)[:s]
    return stream

def pe_sections(pe):
    d = open(pe, 'rb').read(4096)
    peo = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, peo + 6)[0]
    optsz = struct.unpack_from('<H', d, peo + 20)[0]
    so = peo + 24 + optsz
    secs = []
    for i in range(nsec):
        name, vsize, va, rawsize, rawptr = struct.unpack_from('<8sIIII', d, so + i * 40)
        secs.append((name.rstrip(b'\0').decode(), va, vsize, rawptr, rawsize))
    return secs

def main():
    pdb, pe, out = sys.argv[1:4]
    stream = read_msf(pdb)
    dbi = stream(3)
    (vsig, vhdr, age, gsi, build, psi, dllver, symrec) = struct.unpack_from('<iIIHHHHH', dbi, 0)
    recs = stream(symrec)
    secs = pe_sections(pe)
    n = 0
    with open(out, 'w', encoding='utf-8') as o:
        i = 0
        L = len(recs)
        while i + 4 <= L:
            reclen, rectype = struct.unpack_from('<HH', recs, i)
            if rectype == 0x110E:  # S_PUB32
                flags, offset, seg = struct.unpack_from('<IIH', recs, i + 4)
                name = recs[i + 14:i + 2 + reclen].split(b'\0')[0].decode('latin-1')
                if 1 <= seg <= len(secs):
                    rva = secs[seg - 1][1] + offset
                    o.write('%08X\t%d\t%s\n' % (rva, flags, name)); n += 1
            i += 2 + reclen
    print('publics:', n)

main()
