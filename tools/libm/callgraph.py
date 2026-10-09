"""Reachable-function size report from export roots, using .pdata for function bounds (WP-00L research)."""
import struct, sys, re
import capstone
sys.argv=[sys.argv[0]]+sys.argv[1:]
exec(open('pe_tool.py').read().split('def main')[0])
path=sys.argv[1]; roots=sys.argv[2:]
d,secs,ib,dd=load(path)
ex=exports(d,secs,dd)
peo=struct.unpack_from('<I',d,0x3c)[0]
# exception dir = data dir index 3
exc_rva,exc_sz=struct.unpack_from('<II',d,dd+8*3)
o=rva2off(secs,exc_rva)
funcs=[]
for i in range(exc_sz//12):
    b,e,u=struct.unpack_from('<III',d,o+12*i); funcs.append((b,e))
funcs.sort()
import bisect
starts=[f[0] for f in funcs]
def fn_of(r):
    i=bisect.bisect_right(starts,r)-1
    return funcs[i] if i>=0 and funcs[i][0]<=r<funcs[i][1] else None
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
seen={};
def walk(r):
    f=fn_of(r)
    if not f or f[0] in seen: return
    seen[f[0]]=f
    code=d[rva2off(secs,f[0]):rva2off(secs,f[0])+f[1]-f[0]]
    for ins in md.disasm(code,ib+f[0]):
        if ins.mnemonic in('call','jmp') and ins.op_str.startswith('0x'):
            t=int(ins.op_str,16)-ib
            g=fn_of(t)
            if g and g[0]!=f[0]: walk(t)
byrva={r:n for n,r in ex.items()}
for rt in roots:
    seen.clear(); walk(ex[rt])
    print(rt, 'funcs', len(seen), 'bytes', sum(e-b for b,e in seen.values()), ' '.join('%x(%d)%s'%(b,e-b,('='+byrva[b]) if b in byrva else '') for b,e in sorted(seen.values())))
