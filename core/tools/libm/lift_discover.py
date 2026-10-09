"""Function discovery for the msvcr120 lifter (WP-00L). Shared by gen_lift.py."""
import struct, bisect, sys
import capstone
from capstone import x86_const as X
exec(open(__file__.replace('lift_discover.py','pe_tool.py')).read().split('def main')[0])

ROOTS = ['sin_unused']  # placeholder, real roots passed by caller

class Image:
    def __init__(self, path):
        self.d, self.secs, self.ib, self.dd = load(path)
        self.ex = exports(self.d, self.secs, self.dd)
        er, es = struct.unpack_from('<II', self.d, self.dd + 8 * 3)
        o = rva2off(self.secs, er)
        self.pdata = sorted(struct.unpack_from('<III', self.d, o + 12 * i)[0] for i in range(es // 12))
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
        self.md.detail = True
        self.cache = {}
    def decode(self, rva):
        if rva in self.cache: return self.cache[rva]
        o = rva2off(self.secs, rva)
        ins = next(self.md.disasm(self.d[o:o + 16], self.ib + rva, 1), None)
        self.cache[rva] = ins
        return ins
    def mem(self, rva, n):
        o = rva2off(self.secs, rva)
        return self.d[o:o + n] if o is not None else None

def scan(img, entry, entries, stubs):
    """Return (insns{rva:ins}, callees set, tails set, jump targets set)."""
    insns = {}; callees = set(); tails = set(); targets = set()
    work = [entry]
    while work:
        a = work.pop()
        while True:
            if a in insns: break
            ins = img.decode(a)
            if ins is None: raise RuntimeError('undecodable at %x' % a)
            insns[a] = ins
            mn = ins.mnemonic
            nxt = a + ins.size
            if mn in ('ret', 'int3', 'ud2', 'hlt'): break
            if mn == 'jmp' or mn.startswith('j'):
                op = ins.operands[0]
                if op.type != X.X86_OP_IMM:
                    if mn == 'jmp' and op.type == X.X86_OP_MEM:
                        tails.add(('ind', a)); break   # import thunk / external tail call
                    raise RuntimeError('indirect jump at %x: %s %s' % (a, mn, ins.op_str))
                t = op.imm - img.ib
                if mn == 'jmp' and ((t in entries or t in stubs) and t != entry):
                    tails.add(t); break
                targets.add(t); work.append(t)
                if mn == 'jmp': break
            elif mn == 'call':
                op = ins.operands[0]
                if op.type == X.X86_OP_IMM: callees.add(op.imm - img.ib)
                else: callees.add(('ind', a))
            a = nxt
    return insns, callees, tails, targets

EXT = []

def discover(img, roots, stubs):
    entries = set(img.pdata) | set(roots)
    while True:
        funcs = {}; todo = list(roots); seen = set()
        calls_all = set()
        while todo:
            e = todo.pop()
            if e in seen or e in stubs: continue
            seen.add(e)
            insns, callees, tails, targets = scan(img, e, entries, stubs)
            funcs[e] = (insns, callees, tails, targets)
            for c in list(callees) + list(tails):
                if not isinstance(c, int): EXT.append((e, c))
                if isinstance(c, int):
                    calls_all.add(c)
                    if c not in stubs: todo.append(c)
        new = calls_all - entries
        if not new: return funcs
        entries |= new
