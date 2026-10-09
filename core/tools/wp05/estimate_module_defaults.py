"""Diagnostic for WP-06 (not a gate): how many C9 module_fields rows outside WP-05's scope (KSPField values the part
cfg does not set) equal the field's literal C# initializer from include/ksp/parts/module_schema.inc.

    python -I tools/wp05/estimate_module_defaults.py [capture.json] [scope.json]

Prints counts (match / mismatch / unevaluated) and the most frequent mismatches (module OnAwake/OnLoad behaviour,
localised strings, EXPERIMENT definitions...). R32 is an approximation of float.ToString("R").
"""
import json, re, collections, struct
import os, sys
CORE = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
_root = CORE
while not os.path.exists(os.path.join(_root, 'harness', 'compare', 'compare.py')) and os.path.dirname(_root) != _root:
    _root = os.path.dirname(_root)
CAPTURE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(_root, 'harness', 'captures', 'out2', 'part_database.json')
SCOPE = sys.argv[2] if len(sys.argv) > 2 else os.path.join(CORE, 'build', 'RelWithDebInfo', 'candidate', 'part_database', 'scope.json')
inc = open(os.path.join(CORE, 'include', 'ksp', 'parts', 'module_schema.inc'), encoding='utf-8').read().splitlines()
classes = {}; cur = None
CL = re.compile(r'KSP_MODULE_CLASS\((\w+), "([^"]*)", "([^"]*)", "([^"]*)"')
FL = re.compile(r'KSP_MODULE_FIELD\((\w+), "([^"]*)", "([^"]*)", "([^"]*)", (\w+), (\w+), (\w+), "((?:[^"\\]|\\.)*)", (\d+), ([^)]*)\)')
for l in inc:
    m = CL.match(l)
    if m:
        cur = {'name': m.group(2), 'base': m.group(4), 'fields': []}; classes.setdefault(m.group(2), []).append(cur); continue
    m = FL.match(l)
    if m:
        cur['fields'].append(dict(name=m.group(2), cs=m.group(3), tn=m.group(4), init=m.group(8).replace('\\"', '"'), priv=m.group(7) == 'true'))
def allf(name):
    c = classes[name][-1]; out = list(c['fields']); b = c['base']
    while b in classes:
        bc = classes[b][-1]; out += [f for f in bc['fields'] if not f['priv']]; b = bc['base']
    return out
def f32(x): return struct.unpack('f', struct.pack('f', x))[0]
def R32(x):
    # shortest repr that round-trips float32 (approx of .NET R for most values)
    for p in range(6, 10):
        s = '%.*g' % (p, x)
        if f32(float(s)) == x: break
    if 'e' in s:
        m, e = s.split('e'); e = int(e); s = m + 'E' + ('+' if e >= 0 else '-') + ('%02d' % abs(e))
    return s
def evaluate(f):
    t, i = f['tn'], f['init'].strip()
    if i == '':
        return {'Single': '0', 'Double': '0', 'Int32': '0', 'UInt32': '0', 'Boolean': 'False', 'String': 'null', 'Vector3': '0,0,0', 'Vector2': '0,0'}.get(t)
    if t == 'Boolean': return {'true': 'True', 'false': 'False'}.get(i)
    if t == 'String':
        if re.fullmatch(r'"[^"]*"', i): return i[1:-1]
        if i == 'string.Empty': return ''
        return None
    if t in ('Single',):
        try: return R32(f32(float(i.rstrip('fF'))))
        except ValueError: return None
    if t == 'Double':
        try:
            v = float(i.rstrip('dD'));
            return repr(v) if v != int(v) else str(int(v))
        except ValueError: return None
    if t in ('Int32', 'UInt32'):
        try: return str(int(i.rstrip('uU'), 0))
        except ValueError: return None
    return None
d = json.load(open(CAPTURE, encoding='utf-8'))
T = {t['name']: t for t in d['tables']}
def rows(n):
    cols = [c['name'] for c in T[n]['columns']]; return [dict(zip(cols, r)) for r in T[n]['rows']]
M = {(m['part'], m['module_index']): m for m in rows('modules')}
scope = set(tuple(k) for k in json.load(open(SCOPE))['module_fields'])
stats = collections.Counter(); bad = collections.Counter(); unk = collections.Counter()
for r in rows('module_fields'):
    k = (r['part'], r['module_index'], r['field'])
    if k in scope: stats['scope'] += 1; continue
    cls = M[(r['part'], r['module_index'])]['class']
    f = [x for x in allf(cls) if x['name'] == r['field']][0]
    v = evaluate(f)
    if v is None: stats['unevaluated'] += 1; unk[(f['tn'], f['init'][:40])] += 1
    elif v == r['value']: stats['match'] += 1
    else: stats['mismatch'] += 1; bad[(cls, r['field'], f['init'][:30], r['value'][:30])] += 1
print(stats)
print('unevaluated top', unk.most_common(15))
print('mismatch top', bad.most_common(25))
