import re, sys, collections
src = open(sys.argv[1], encoding='utf-8').read().splitlines()
decl = re.compile(r'^\s*public static (?:readonly )?([\w<>,\. \[\]]+?) (\w+) = new ([\w<>,\. \[\]]+?)\((.*)\);\s*$')
cls = re.compile(r'^\s*public static class (\w+)')
depth = 0; stack = []
n = 0; types = collections.Counter(); kinds = collections.Counter(); other = []
for i, line in enumerate(src, 1):
    m = cls.match(line)
    if m: stack.append((m.group(1), depth))
    d = decl.match(line)
    if d:
        n += 1
        t = d.group(1)
        kinds[re.sub(r'<.*', '', t)] += 1
        inner = re.search(r'<(.*)>', t)
        if inner:
            # split top-level commas
            s = inner.group(1); parts=[]; lvl=0; cur=''
            for ch in s:
                if ch=='<': lvl+=1
                if ch=='>': lvl-=1
                if ch==',' and lvl==0: parts.append(cur.strip()); cur=''
                else: cur+=ch
            parts.append(cur.strip())
            for p in parts: types[p]+=1
    elif 'public static' in line and '(' not in line.split('=')[0]:
        other.append((i, line.strip()))
    depth += line.count('{') - line.count('}')
    while stack and depth <= stack[-1][1]: stack.pop()
print('decls', n)
for k,v in kinds.most_common(): print('KIND', k, v)
for k,v in sorted(types.items()): print('TYPE', k, v)
for o in other: print('OTHER', o)
