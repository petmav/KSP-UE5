"""Lifts the x64 libm functions of msvcr120_clr0400.dll into portable C++ (WP-00L).

Usage (venv python with capstone):  python gen_lift.py <msvcr120_clr0400.dll> <out_dir>
Writes <out_dir>/vs2013_libm_gen.cpp (translated functions) and refs.txt (rip-relative data references).
The translation is mechanical: one C++ statement block per x86 instruction over lift::Cpu (see vs2013_lift.hpp).
"""
import sys, os, struct, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lift_discover import *
from capstone import x86_const as X

NAMES = ['acos', 'asin', 'atan', 'atan2', 'cosh', 'exp', 'log', 'log10', 'pow', 'sinh', 'tan', 'tanh']
# CRT infrastructure: calls to these cannot change a numeric result (FP status word, errno, _matherr hook, cookie check)
STUBS = {
    0xa2488: 'StubHandleError',  # _handle_error(name, op, retval, ...): returns retval in xmm0
    0xa2330: 'StubRet0', 0xa22b4: 'StubRet0', 0xa2294: 'StubRet0', 0xa2350: 'StubRet0', 0xa2364: 'StubRet0',
    0xa23cc: 'StubRet0', 0xa56d0: 'StubRet0', 0xa2710: 'StubRet0', 0xa271a: 'StubRet0', 0xa5a28: 'StubRet0',
    0xa2700: 'StubGetMxcsr', 0x297d0: 'StubErrno', 0x24520: 'StubRet0', 0x72230: 'StubRet0',
}

GPR = {}
for i, n in enumerate(['ax', 'cx', 'dx', 'bx', 'sp', 'bp', 'si', 'di']):
    GPR['r' + n] = (i, 64, 0); GPR['e' + n] = (i, 32, 0); GPR[n] = (i, 16, 0)
for n, i in [('al', 0), ('cl', 1), ('dl', 2), ('bl', 3)]: GPR[n] = (i, 8, 0)
for n, i in [('ah', 0), ('ch', 1), ('dh', 2), ('bh', 3)]: GPR[n] = (i, 8, 8)
for n, i in [('spl', 4), ('bpl', 5), ('sil', 6), ('dil', 7)]: GPR[n] = (i, 8, 0)
for i in range(8, 16):
    GPR['r%d' % i] = (i, 64, 0); GPR['r%dd' % i] = (i, 32, 0); GPR['r%dw' % i] = (i, 16, 0); GPR['r%db' % i] = (i, 8, 0)

CC = {'z': 'e', 'nz': 'ne', 'c': 'b', 'nc': 'ae', 'na': 'be', 'nbe': 'a', 'nb': 'ae', 'nae': 'b', 'pe': 'p', 'po': 'np',
      'nle': 'g', 'nl': 'ge', 'ng': 'le', 'nge': 'l'}
COND = {'e': 'c.zf', 'ne': '!c.zf', 'b': 'c.cf', 'ae': '!c.cf', 'be': '(c.cf || c.zf)', 'a': '(!c.cf && !c.zf)',
        's': 'c.sf', 'ns': '!c.sf', 'p': 'c.pf', 'np': '!c.pf', 'l': '(c.sf != c.of)', 'ge': '(c.sf == c.of)',
        'le': '(c.zf || c.sf != c.of)', 'g': '(!c.zf && c.sf == c.of)', 'o': 'c.of', 'no': '!c.of'}


def M(b):
    return '0x%xull' % ((1 << b) - 1) if b < 64 else '~0ull'


class Lift:
    def __init__(self, img):
        self.img = img
        self.rip_refs = []   # (insn rva, target VA, size)
        self.disp_refs = set()  # absolute displacements of [reg + index*scale + disp] operands (table bases: reg = image base)

    def regname(self, r): return self.img.md.reg_name(r)

    def isxmm(self, r):
        n = self.regname(r)
        return n.startswith('xmm') or n.startswith('ymm')

    def xi(self, r): return int(self.regname(r)[3:])

    # --- operand expression helpers ---
    def addr(self, ins, op):
        m = op.mem
        parts = []
        if m.segment:
            raise RuntimeError('segment override at %x' % (ins.address - self.img.ib))
        if m.base:
            bn = self.regname(m.base)
            if bn == 'rip':
                va = ins.address + ins.size + m.disp
                self.rip_refs.append((ins.address - self.img.ib, va, op.size))
                return '0x%xull' % va
            parts.append('c.r[%d]' % GPR[bn][0])
        if m.index:
            idx = 'c.r[%d]' % GPR[self.regname(m.index)][0]
            parts.append('%s * %d' % (idx, m.scale) if m.scale != 1 else idx)
        if (m.base or m.index) and m.disp >= 0x1000:
            self.disp_refs.add(m.disp)
        if m.disp or not parts:
            parts.append('0x%xull' % (m.disp & 0xffffffffffffffff))
        return '(' + ' + '.join(parts) + ')'

    def rd(self, ins, op, bits=None):
        """unsigned value of a GPR/imm/mem operand at its own size"""
        if op.type == X.X86_OP_REG:
            i, b, sh = GPR[self.regname(op.reg)]
            if b == 64: return 'c.r[%d]' % i
            if sh: return '((c.r[%d] >> %d) & 0x%x)' % (i, sh, (1 << b) - 1)
            return '(c.r[%d] & 0x%xull)' % (i, (1 << b) - 1)
        if op.type == X.X86_OP_IMM:
            b = bits or op.size * 8
            return '0x%xull' % (op.imm & ((1 << b) - 1))
        if op.type == X.X86_OP_MEM:
            return 'Ld(c, %s, %d)' % (self.addr(ins, op), op.size)
        raise RuntimeError('rd')

    def wr(self, ins, op, expr):
        if op.type == X.X86_OP_REG:
            i, b, sh = GPR[self.regname(op.reg)]
            if b == 64: return 'c.r[%d] = %s;' % (i, expr)
            if b == 32: return 'c.r[%d] = (uint32_t)(%s);' % (i, expr)
            m = (1 << b) - 1
            return 'c.r[%d] = (c.r[%d] & ~0x%xull) | (((uint64_t)(%s) & 0x%xull) << %d);' % (i, i, m << sh, expr, m, sh)
        if op.type == X.X86_OP_MEM:
            return 'St(c, %s, %s, %d);' % (self.addr(ins, op), expr, op.size)
        raise RuntimeError('wr')

    def xlo(self, ins, op):
        if op.type == X.X86_OP_REG: return 'c.x[%d].lo' % self.xi(op.reg)
        if op.type == X.X86_OP_MEM: return 'Ld(c, %s, 8)' % self.addr(ins, op)
        raise RuntimeError('xlo')

    def xd(self, ins, op): return 'D(%s)' % self.xlo(ins, op)

    def xf(self, ins, op):
        if op.type == X.X86_OP_REG: return 'F(c.x[%d].lo)' % self.xi(op.reg)
        if op.type == X.X86_OP_MEM: return 'F(Ld(c, %s, 4))' % self.addr(ins, op)
        raise RuntimeError('xf')

    def x128(self, ins, op):
        if op.type == X.X86_OP_REG: return 'c.x[%d]' % self.xi(op.reg)
        if op.type == X.X86_OP_MEM:
            if op.size in (4, 8): return 'X128{Ld(c, %s, %d), 0}' % (self.addr(ins, op), op.size)
            return 'Ld128(c, %s)' % self.addr(ins, op)
        raise RuntimeError('x128')

    # --- instruction translation ---
    def tr(self, ins, label_of, fname):
        mn = ins.mnemonic
        ops = ins.operands
        rva = ins.address - self.img.ib
        nxt = rva + ins.size

        def W(op, e): return self.wr(ins, op, e)

        def bits(op): return op.size * 8

        # ---------- control flow ----------
        if mn in ('nop', 'int3'): return ''
        if mn == 'ret':
            return 'c.r[RSP] += 8; return;'
        if mn == 'jmp':
            t = ops[0].imm - self.img.ib
            if t in label_of: return 'goto %s;' % label_of[t]
            if t in STUBS: return '%s(c); c.r[RSP] += 8; return;' % STUBS[t]
            return 'f_%x(c); return;' % t
        if mn.startswith('j'):
            t = ops[0].imm - self.img.ib
            cc = mn[1:]
            cc = CC.get(cc, cc)
            return 'if (%s) goto %s;' % (COND[cc], label_of[t])
        if mn == 'call':
            if ops[0].type != X.X86_OP_IMM:
                raise RuntimeError('indirect call at %x in %s' % (rva, fname))
            t = ops[0].imm - self.img.ib
            if t in STUBS: return '%s(c);' % STUBS[t]
            return 'c.r[RSP] -= 8; St(c, c.r[RSP], 0x%xull, 8); f_%x(c);' % (nxt, t)
        if mn == 'push':
            if ops[0].type == X.X86_OP_IMM:
                v = '0x%xull' % (ops[0].imm & 0xffffffffffffffff)
            else:
                v = self.rd(ins, ops[0], 64)
            return 'c.r[RSP] -= 8; St(c, c.r[RSP], %s, 8);' % v
        if mn == 'pop':
            return '{ uint64_t v = Ld(c, c.r[RSP], 8); c.r[RSP] += 8; %s }' % W(ops[0], 'v')
        # ---------- integer ----------
        if mn in ('mov', 'movabs'):
            return W(ops[0], self.rd(ins, ops[1], bits(ops[0])))
        if mn == 'movsxd':
            return W(ops[0], '(uint64_t)(int64_t)(int32_t)%s' % self.rd(ins, ops[1]))
        if mn == 'movzx':
            return W(ops[0], self.rd(ins, ops[1]))
        if mn == 'movsx':
            return W(ops[0], '(uint64_t)SExt(%s, %d)' % (self.rd(ins, ops[1]), bits(ops[1])))
        if mn == 'lea':
            return W(ops[0], self.addr(ins, ops[1]))
        if mn in ('add', 'sub', 'and', 'or', 'xor', 'cmp', 'test', 'adc'):
            b = bits(ops[0])
            a_ = self.rd(ins, ops[0])
            b_ = self.rd(ins, ops[1], b)
            if ops[1].type == X.X86_OP_IMM:
                b_ = '0x%xull' % (ops[1].imm & ((1 << b) - 1))   # sign-extended immediate at the operand width
            m_ = M(b)
            if mn == 'add':
                return '{ uint64_t a = %s, b = %s, r = (a + b) & %s; FlAdd(c, a, b, 0, r, %d); %s }' % (a_, b_, m_, b, W(ops[0], 'r'))
            if mn == 'adc':
                return '{ uint64_t a = %s, b = %s, ci = c.cf ? 1 : 0, r = (a + b + ci) & %s; FlAdd(c, a, b, ci, r, %d); %s }' % (a_, b_, m_, b, W(ops[0], 'r'))
            if mn == 'sub':
                return '{ uint64_t a = %s, b = %s, r = (a - b) & %s; FlSub(c, a, b, r, %d); %s }' % (a_, b_, m_, b, W(ops[0], 'r'))
            if mn == 'cmp':
                return '{ uint64_t a = %s, b = %s, r = (a - b) & %s; FlSub(c, a, b, r, %d); }' % (a_, b_, m_, b)
            if mn == 'test':
                return '{ uint64_t r = %s & %s; FlLogic(c, r, %d); }' % (a_, b_, b)
            op = {'and': '&', 'or': '|', 'xor': '^'}[mn]
            return '{ uint64_t r = (%s %s %s) & %s; FlLogic(c, r, %d); %s }' % (a_, op, b_, m_, b, W(ops[0], 'r'))
        if mn in ('inc', 'dec'):
            b = bits(ops[0])
            if mn == 'inc':
                return '{ bool cf = c.cf; uint64_t a = %s, r = (a + 1) & %s; FlAdd(c, a, 1, 0, r, %d); c.cf = cf; %s }' % (self.rd(ins, ops[0]), M(b), b, W(ops[0], 'r'))
            return '{ bool cf = c.cf; uint64_t a = %s, r = (a - 1) & %s; FlSub(c, a, 1, r, %d); c.cf = cf; %s }' % (self.rd(ins, ops[0]), M(b), b, W(ops[0], 'r'))
        if mn == 'neg':
            b = bits(ops[0])
            return '{ uint64_t a = %s, r = (0 - a) & %s; FlSub(c, 0, a, r, %d); %s }' % (self.rd(ins, ops[0]), M(b), b, W(ops[0], 'r'))
        if mn == 'not':
            b = bits(ops[0])
            return W(ops[0], '(~%s) & %s' % (self.rd(ins, ops[0]), M(b)))
        if mn in ('shl', 'shr', 'sar'):
            b = bits(ops[0])
            if len(ops) > 1 and ops[1].type == X.X86_OP_IMM:
                cnt = str(ops[1].imm & (63 if b == 64 else 31))
            elif len(ops) > 1:
                cnt = '(%s & %d)' % (self.rd(ins, ops[1], 8), 63 if b == 64 else 31)
            else:
                cnt = '1'
            if mn == 'shl':
                body = 'r = (a << n) & %s; c.cf = ((a >> (%d - n)) & 1) != 0; c.of = (((r >> %d) & 1) != 0) != c.cf;' % (M(b), b, b - 1)
            elif mn == 'shr':
                body = 'r = a >> n; c.cf = ((a >> (n - 1)) & 1) != 0; c.of = (a >> %d) != 0;' % (b - 1)
            else:
                body = 'r = (uint64_t)(SExt(a, %d) >> n) & %s; c.cf = ((uint64_t)(SExt(a, %d) >> (n - 1)) & 1) != 0; c.of = false;' % (b, M(b), b)
            return ('{ uint64_t a = %s, n = %s, r = a; if (n) { %s c.zf = r == 0; c.sf = (r & SignBit(%d)) != 0; c.pf = Parity(r); } %s }'
                    % (self.rd(ins, ops[0]), cnt, body, b, W(ops[0], 'r')))
        if mn in ('bt', 'btr', 'bts'):
            b = bits(ops[0])
            if ops[0].type == X.X86_OP_MEM: raise RuntimeError('bt mem')
            n = '(%s & %d)' % (self.rd(ins, ops[1], b), b - 1)
            base = self.rd(ins, ops[0])
            if mn == 'bt': return '{ c.cf = ((%s >> %s) & 1) != 0; }' % (base, n)
            nv = '(a & ~(1ull << n))' if mn == 'btr' else '(a | (1ull << n))'
            return '{ uint64_t a = %s, n = %s; c.cf = ((a >> n) & 1) != 0; %s }' % (base, n, W(ops[0], nv))
        if mn == 'bsr':
            return '{ uint64_t v = %s; c.zf = v == 0; if (v) { %s } }' % (self.rd(ins, ops[1]), W(ops[0], 'Bsr(v)'))
        if mn == 'mul':
            b = bits(ops[0])
            if b == 64:
                return '{ uint64_t hi, lo = MulWide(c.r[RAX], %s, hi); c.r[RAX] = lo; c.r[RDX] = hi; c.cf = c.of = hi != 0; }' % self.rd(ins, ops[0])
            if b == 32:
                return '{ uint64_t p = (c.r[RAX] & 0xffffffffull) * %s; c.r[RAX] = (uint32_t)p; c.r[RDX] = (uint32_t)(p >> 32); c.cf = c.of = (p >> 32) != 0; }' % self.rd(ins, ops[0])
            raise RuntimeError('mul %d' % b)
        if mn == 'cdq': return 'c.r[RDX] = (c.r[RAX] & 0x80000000ull) ? 0xffffffffull : 0;'
        if mn == 'cdqe': return 'c.r[RAX] = (uint64_t)(int64_t)(int32_t)c.r[RAX];'
        if mn == 'cqo': return 'c.r[RDX] = (c.r[RAX] >> 63) ? ~0ull : 0;'
        if mn.startswith('set'):
            cc = CC.get(mn[3:], mn[3:])
            return W(ops[0], '(%s) ? 1 : 0' % COND[cc])
        if mn.startswith('cmov'):
            cc = CC.get(mn[4:], mn[4:])
            b = bits(ops[0])
            if b == 32:   # 32-bit cmov zero-extends the destination even when the condition is false
                return 'if (%s) { %s } else { c.r[%d] &= 0xffffffffull; }' % (COND[cc], W(ops[0], self.rd(ins, ops[1])), GPR[self.regname(ops[0].reg)][0])
            return 'if (%s) { %s }' % (COND[cc], W(ops[0], self.rd(ins, ops[1])))
        # ---------- SSE / AVX ----------
        v = mn.startswith('v') and not mn.startswith('vf')
        base = mn[1:] if v else mn

        def three():
            return (ops[0], ops[1], ops[2]) if (v and len(ops) == 3) else (ops[0], ops[0], ops[1])

        def d_(i): return 'c.x[%d]' % self.xi(i.reg)

        if base in ('addsd', 'subsd', 'mulsd', 'divsd'):
            dst, a1, a2 = three()
            opch = {'addsd': '+', 'subsd': '-', 'mulsd': '*', 'divsd': '/'}[base]
            return '{ X128 s1 = %s; double r = D(s1.lo) %s %s; %s = {B(r), s1.hi}; }' % (self.x128(ins, a1), opch, self.xd(ins, a2), d_(dst))
        if base == 'sqrtsd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s; double r = std::sqrt(%s); %s = {B(r), s1.hi}; }' % (self.x128(ins, a1), self.xd(ins, a2), d_(dst))
        if base == 'movsd':
            if ops[0].type == X.X86_OP_MEM: return 'St(c, %s, %s, 8);' % (self.addr(ins, ops[0]), self.xlo(ins, ops[1]))
            if len(ops) == 3: return '{ X128 s1 = %s; %s = {%s, s1.hi}; }' % (self.x128(ins, ops[1]), d_(ops[0]), self.xlo(ins, ops[2]))
            if ops[1].type == X.X86_OP_MEM: return '%s = {%s, 0};' % (d_(ops[0]), self.xlo(ins, ops[1]))
            return '%s.lo = %s;' % (d_(ops[0]), self.xlo(ins, ops[1]))
        if base in ('movaps', 'movapd', 'movdqa', 'movdqu', 'movups', 'movupd'):
            if ops[0].type == X.X86_OP_MEM: return 'St128(c, %s, %s);' % (self.addr(ins, ops[0]), self.x128(ins, ops[1]))
            return '%s = %s;' % (d_(ops[0]), self.x128(ins, ops[1]))
        if base in ('movq', 'movd'):
            n = 8 if base == 'movq' else 4
            dst, src = ops[0], ops[1]
            if dst.type == X.X86_OP_REG and self.isxmm(dst.reg):
                if src.type == X.X86_OP_REG and self.isxmm(src.reg):
                    sv = 'c.x[%d].lo' % self.xi(src.reg)
                elif src.type == X.X86_OP_MEM:
                    sv = 'Ld(c, %s, %d)' % (self.addr(ins, src), n)
                else:
                    sv = self.rd(ins, src)
                if n == 4: sv = '((%s) & 0xffffffffull)' % sv
                return '%s = {%s, 0};' % (d_(dst), sv)
            xs = 'c.x[%d].lo' % self.xi(src.reg)
            if n == 4: xs = '(%s & 0xffffffffull)' % xs
            return W(dst, xs)
        if base in ('xorps', 'xorpd', 'pxor', 'andps', 'andpd', 'pand', 'orps', 'orpd', 'por', 'andnps', 'andnpd', 'pandn'):
            dst, a1, a2 = three()
            if 'andn' in base:
                return '{ X128 s1 = %s, s2 = %s; %s = {~s1.lo & s2.lo, ~s1.hi & s2.hi}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
            opch = '^' if 'xor' in base else ('|' if base in ('orps', 'orpd', 'por') else '&')
            return '{ X128 s1 = %s, s2 = %s; %s = {s1.lo %s s2.lo, s1.hi %s s2.hi}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst), opch, opch)
        if base in ('paddq', 'psubq'):
            dst, a1, a2 = three()
            ch = '+' if base == 'paddq' else '-'
            return '{ X128 s1 = %s, s2 = %s; %s = {s1.lo %s s2.lo, s1.hi %s s2.hi}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst), ch, ch)
        if base == 'psubd':
            dst, a1, a2 = three()
            return ('{ X128 s1 = %s, s2 = %s; auto f = [](uint64_t a, uint64_t b) { return ((a - b) & 0xffffffffull) | ((((a >> 32) - (b >> 32)) & 0xffffffffull) << 32); }; %s = {f(s1.lo, s2.lo), f(s1.hi, s2.hi)}; }'
                    % (self.x128(ins, a1), self.x128(ins, a2), d_(dst)))
        if base in ('psrlq', 'psllq'):
            dst, a1, a2 = three()
            ch = '>>' if base == 'psrlq' else '<<'
            cnt = str(a2.imm) if a2.type == X.X86_OP_IMM else '(%s)' % self.xlo(ins, a2)
            return '{ X128 s1 = %s; uint64_t n = %s; %s = n > 63 ? X128{0, 0} : X128{s1.lo %s n, s1.hi %s n}; }' % (self.x128(ins, a1), cnt, d_(dst), ch, ch)
        if base == 'psrldq':
            dst, a1, a2 = three()
            n = a2.imm
            return '{ X128 s1 = %s; uint8_t b[32] = {0}; std::memcpy(b, &s1, 16); X128 r; std::memcpy(&r, b + %d, 16); %s = r; }' % (self.x128(ins, a1), min(n, 16), d_(dst))
        if base in ('comisd', 'ucomisd'):
            return 'FlComisd(c, %s, %s);' % (self.xd(ins, ops[0]), self.xd(ins, ops[1]))
        if base in ('cmplesd', 'cmpnltsd'):
            dst, a1, a2 = three()
            e = 'D(s1.lo) <= %s' % self.xd(ins, a2) if base == 'cmplesd' else '!(D(s1.lo) < %s)' % self.xd(ins, a2)
            return '{ X128 s1 = %s; bool t = %s; %s = {t ? ~0ull : 0ull, s1.hi}; }' % (self.x128(ins, a1), e, d_(dst))
        if base == 'cvtsi2sd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s; %s = {B((double)SExt(%s, %d)), s1.hi}; }' % (self.x128(ins, a1), d_(dst), self.rd(ins, a2), bits(a2))
        if base == 'cvttsd2si':
            return W(ops[0], 'Cvttsd2si(%s, %d)' % (self.xd(ins, ops[1]), bits(ops[0])))
        if base == 'cvtdq2pd':
            return ('{ uint64_t s = %s; %s = {B((double)(int32_t)(uint32_t)s), B((double)(int32_t)(uint32_t)(s >> 32))}; }'
                    % (self.xlo(ins, ops[1]), d_(ops[0])))
        if base == 'cvttpd2dq':
            return '{ X128 s = %s; %s = {Cvttsd2si(D(s.lo), 32) | (Cvttsd2si(D(s.hi), 32) << 32), 0}; }' % (self.x128(ins, ops[1]), d_(ops[0]))
        if base == 'cvtpd2dq':
            return '{ X128 s = %s; %s = {(uint64_t)Cvtsd2dq(D(s.lo)) | ((uint64_t)Cvtsd2dq(D(s.hi)) << 32), 0}; }' % (self.x128(ins, ops[1]), d_(ops[0]))
        if base == 'mulpd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {B(D(s1.lo) * D(s2.lo)), B(D(s1.hi) * D(s2.hi))}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base == 'movhlps':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {s2.hi, s1.hi}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base == 'movlhps':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {s1.lo, s2.lo}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base == 'haddpd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {B(D(s1.lo) + D(s1.hi)), B(D(s2.lo) + D(s2.hi))}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base in ('addss', 'subss', 'mulss', 'divss', 'sqrtss'):
            dst, a1, a2 = three()
            fs = self.xf(ins, a2)
            if base == 'sqrtss':
                e = 'std::sqrt(%s)' % fs
            else:
                e = 'F(s1.lo) %s %s' % ({'addss': '+', 'subss': '-', 'mulss': '*', 'divss': '/'}[base], fs)
            return '{ X128 s1 = %s; float r = %s; %s = {MergeLo32(s1.lo, r), s1.hi}; }' % (self.x128(ins, a1), e, d_(dst))
        if base == 'movss':
            if ops[0].type == X.X86_OP_MEM: return 'St(c, %s, %s & 0xffffffffull, 4);' % (self.addr(ins, ops[0]), self.xlo(ins, ops[1]))
            if len(ops) == 3: return '{ X128 s1 = %s; %s = {(s1.lo & ~0xffffffffull) | (%s & 0xffffffffull), s1.hi}; }' % (self.x128(ins, ops[1]), d_(ops[0]), self.xlo(ins, ops[2]))
            if ops[1].type == X.X86_OP_MEM: return '%s = {Ld(c, %s, 4), 0};' % (d_(ops[0]), self.addr(ins, ops[1]))
            return '%s.lo = (%s.lo & ~0xffffffffull) | (%s.lo & 0xffffffffull);' % (d_(ops[0]), d_(ops[0]), d_(ops[1]))
        if base == 'cvtss2sd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s; %s = {B((double)%s), s1.hi}; }' % (self.x128(ins, a1), d_(dst), self.xf(ins, a2))
        if base == 'cvtsd2ss':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s; float r = (float)%s; %s = {MergeLo32(s1.lo, r), s1.hi}; }' % (self.x128(ins, a1), self.xd(ins, a2), d_(dst))
        if base in ('comiss', 'ucomiss'):
            return 'FlComisd(c, (double)%s, (double)%s);' % (self.xf(ins, ops[0]), self.xf(ins, ops[1]))
        if base == 'cvttss2si':
            return W(ops[0], 'Cvttsd2si((double)%s, %d)' % (self.xf(ins, ops[1]), bits(ops[0])))
        if base == 'cvtsi2ss':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s; float r = (float)SExt(%s, %d); %s = {MergeLo32(s1.lo, r), s1.hi}; }' % (self.x128(ins, a1), self.rd(ins, a2), bits(a2), d_(dst))
        if mn == 'ldmxcsr':
            return 'c.mxcsr = (uint32_t)Ld(c, %s, 4);' % self.addr(ins, ops[0])
        if base == 'cvtss2si':
            return W(ops[0], 'CvtToInt((double)%s, (c.mxcsr >> 13) & 3, %d)' % (self.xf(ins, ops[1]), bits(ops[0])))
        if base == 'roundss':
            dst, a1, a2 = three() if v else (ops[0], ops[0], ops[1])
            imm = ops[-1].imm
            rc = '((c.mxcsr >> 13) & 3)' if imm & 4 else str(imm & 3)
            src = ops[-2]
            return '{ X128 s1 = %s; float r = (float)RoundMode((double)%s, %s); %s = {MergeLo32(s1.lo, r), s1.hi}; }' % (self.x128(ins, a1), self.xf(ins, src), rc, d_(dst))
        if mn == 'rcr':
            b = bits(ops[0])
            assert ops[1].type == X.X86_OP_IMM and ops[1].imm == 1
            return ('{ uint64_t a = %s, r = ((c.cf ? 1ull : 0ull) << %d) | (a >> 1); c.cf = (a & 1) != 0; %s }' % (self.rd(ins, ops[0]), b - 1, W(ops[0], 'r')))
        if base == 'cvtps2pd':
            return '{ uint64_t s = %s; %s = {B((double)F(s)), B((double)F(s >> 32))}; }' % (self.xlo(ins, ops[1]), d_(ops[0]))
        if base == 'pshufd':
            imm = ops[-1].imm
            src = ops[-2]
            return ('{ X128 s = %s; uint32_t w[4] = {(uint32_t)s.lo, (uint32_t)(s.lo >> 32), (uint32_t)s.hi, (uint32_t)(s.hi >> 32)}; '
                    '%s = {(uint64_t)w[%d] | ((uint64_t)w[%d] << 32), (uint64_t)w[%d] | ((uint64_t)w[%d] << 32)}; }'
                    % (self.x128(ins, src), d_(ops[0]), imm & 3, (imm >> 2) & 3, (imm >> 4) & 3, (imm >> 6) & 3))
        if base == 'shufps':
            imm = ops[-1].imm
            dst, a1, a2 = three()
            return ('{ X128 s1 = %s, s2 = %s; uint32_t w1[4] = {(uint32_t)s1.lo, (uint32_t)(s1.lo >> 32), (uint32_t)s1.hi, (uint32_t)(s1.hi >> 32)}; '
                    'uint32_t w2[4] = {(uint32_t)s2.lo, (uint32_t)(s2.lo >> 32), (uint32_t)s2.hi, (uint32_t)(s2.hi >> 32)}; '
                    '%s = {(uint64_t)w1[%d] | ((uint64_t)w1[%d] << 32), (uint64_t)w2[%d] | ((uint64_t)w2[%d] << 32)}; }'
                    % (self.x128(ins, a1), self.x128(ins, a2), d_(dst), imm & 3, (imm >> 2) & 3, (imm >> 4) & 3, (imm >> 6) & 3))
        if base == 'psrad':
            dst, a1, a2 = three()
            n = a2.imm
            return ('{ X128 s1 = %s; auto f = [](uint64_t q) { return (uint64_t)(uint32_t)((int32_t)(uint32_t)q >> %d) | ((uint64_t)(uint32_t)((int32_t)(uint32_t)(q >> 32) >> %d) << 32); }; %s = {f(s1.lo), f(s1.hi)}; }'
                    % (self.x128(ins, a1), min(n, 31), min(n, 31), d_(dst)))
        if base == 'addpd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {B(D(s1.lo) + D(s2.lo)), B(D(s1.hi) + D(s2.hi))}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base == 'unpcklpd':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {s1.lo, s2.lo}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if base == 'punpckldq':
            dst, a1, a2 = three()
            return ('{ X128 s1 = %s, s2 = %s; %s = {(s1.lo & 0xffffffffull) | (s2.lo << 32), (s1.lo >> 32) | (s2.lo & 0xffffffff00000000ull)}; }'
                    % (self.x128(ins, a1), self.x128(ins, a2), d_(dst)))
        if base == 'pmovsxdq':
            return '{ uint64_t s = %s; %s = {(uint64_t)(int64_t)(int32_t)(uint32_t)s, (uint64_t)(int64_t)(int32_t)(uint32_t)(s >> 32)}; }' % (self.xlo(ins, ops[1]), d_(ops[0]))
        if base == 'pcmpeqq':
            dst, a1, a2 = three()
            return '{ X128 s1 = %s, s2 = %s; %s = {s1.lo == s2.lo ? ~0ull : 0ull, s1.hi == s2.hi ? ~0ull : 0ull}; }' % (self.x128(ins, a1), self.x128(ins, a2), d_(dst))
        if mn == 'stmxcsr':
            return 'St(c, %s, c.mxcsr, 4);' % self.addr(ins, ops[0])
        # FMA3 scalar / packed
        mf = re.match(r'^vf(n?)m(add|sub)(132|213|231)(sd|pd|ss)$', mn)
        if mf:
            neg, kind, order, sz = mf.groups()
            x1, x2, x3 = ops[0], ops[1], ops[2]

            def fma_lane(a, b, cc):
                aa = '-(%s)' % a if neg else a
                return 'std::fma(%s, %s, %s)' % (aa, b, ('-(%s)' % cc) if kind == 'sub' else cc)

            def roles(l1, l2, l3):
                # 132: x1*x3 + x2 ; 213: x2*x1 + x3 ; 231: x2*x3 + x1
                return {'132': (l1, l3, l2), '213': (l2, l1, l3), '231': (l2, l3, l1)}[order]
            if sz == 'ss':
                def fl(op_, sfx): return None
                a, b, cc = roles('F(s1.lo)', 'F(s2.lo)', 'F(s3.lo)')
                aa = '-(%s)' % a if neg else a
                e = 'std::fma(%s, %s, %s)' % (aa, b, ('-(%s)' % cc) if kind == 'sub' else cc)
                return '{ X128 s1 = %s, s2 = %s, s3 = %s; float r = %s; %s = {MergeLo32(s1.lo, r), s1.hi}; }' % (self.x128(ins, x1), self.x128(ins, x2), self.x128(ins, x3), e, d_(x1))
            a, b, cc = roles('D(s1.lo)', 'D(s2.lo)', 'D(s3.lo)')
            if sz == 'sd':
                return '{ X128 s1 = %s, s2 = %s, s3 = %s; %s = {B(%s), s1.hi}; }' % (self.x128(ins, x1), self.x128(ins, x2), self.x128(ins, x3), d_(x1), fma_lane(a, b, cc))
            a2_, b2_, c2_ = roles('D(s1.hi)', 'D(s2.hi)', 'D(s3.hi)')
            return '{ X128 s1 = %s, s2 = %s, s3 = %s; %s = {B(%s), B(%s)}; }' % (self.x128(ins, x1), self.x128(ins, x2), self.x128(ins, x3), d_(x1), fma_lane(a, b, cc), fma_lane(a2_, b2_, c2_))
        raise RuntimeError('unsupported %s %s at %x in %s' % (mn, ins.op_str, rva, fname))


def emit(img, funcs):
    lf = Lift(img)
    protos = ['static void f_%x(Cpu& c);' % e for e in sorted(funcs)]
    lines = []
    for e, (insns, callees, tails, targets) in sorted(funcs.items()):
        label_of = {t: 'L_%x' % t for t in targets}
        fname = '%x' % e
        order = sorted(insns)
        lines.append('static void f_%x(Cpu& c) {' % e)
        if order[0] != e:
            label_of[e] = 'L_%x' % e
            lines.append('    goto L_%x;' % e)
        for a in order:
            ins = insns[a]
            if a in label_of: lines.append('L_%x:;' % a)
            t = lf.tr(ins, label_of, fname)
            if t: lines.append('    ' + t)
            nxt = a + ins.size
            if ins.mnemonic not in ('ret', 'jmp', 'int3') and nxt not in insns:
                lines.append('    Trap();  // falls off the decoded range')
        lines.append('}')
        lines.append('')
    return lf, protos, lines


