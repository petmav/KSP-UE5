import sys,collections
sys.argv=[sys.argv[0]]+sys.argv[1:]
src=open('callgraph.py').read().split('byrva={r')[0]
exec(src)
roots=['sinh','cosh','tanh','exp','pow','asin','acos','atan','atan2','tan','log','log10']
allf={}
for rt in roots:
    seen.clear(); walk(ex[rt])
    for b,f in seen.items():
        if b>=0x80000: allf.setdefault(b,f)
print('numeric funcs',len(allf),'bytes',sum(e-b for b,e in allf.values()))
cnt=collections.Counter()
calls=collections.Counter()
for b,(s,e) in sorted(allf.items()):
    code=d[rva2off(secs,s):rva2off(secs,s)+e-s]
    for ins in md.disasm(code,ib+s):
        cnt[ins.mnemonic]+=1
        if ins.mnemonic=='call': calls[ins.op_str]+=1
print(sorted(cnt.items(), key=lambda x:-x[1]))
print(calls)
print(' '.join('%x(%d)'%(b,e-b) for b,(s,e) in sorted(allf.items())))
