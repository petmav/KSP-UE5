"""WP-06 diagnostic (not a gate): compare the candidate part_database.json against the C9 capture row by row for the
module_fields table and group the differences by (module class, field).

    python -I tools/wp06/diff_module_fields.py [candidate.json] [capture.json]

Prints, per (class, field), the number of differing rows and one example (part, capture value -> candidate value).
"""
import collections
import json
import os
import sys

CORE = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
root = CORE
while not os.path.exists(os.path.join(root, 'harness', 'compare', 'compare.py')) and os.path.dirname(root) != root:
    root = os.path.dirname(root)
cand = sys.argv[1] if len(sys.argv) > 1 else os.path.join(CORE, 'build', 'linux-rel', 'candidate', 'part_database', 'part_database.json')
cap = sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, 'harness', 'captures', 'out2', 'part_database.json')


def load(p):
    d = json.load(open(p, encoding='utf-8'))
    T = {t['name']: t for t in d['tables']}
    def rows(n):
        cols = [c['name'] for c in T[n]['columns']]
        return [dict(zip(cols, r)) for r in T[n]['rows']]
    return rows


rc, rg = load(cand), load(cap)
cls = {(m['part'], m['module_index']): m['class'] for m in rg('modules')}
C = {(r['part'], r['module_index'], r['field']): r for r in rc('module_fields')}
stats = collections.Counter()
groups = collections.defaultdict(list)
for r in rg('module_fields'):
    k = (r['part'], r['module_index'], r['field'])
    c = C.get(k)
    if c is None:
        stats['missing'] += 1
        continue
    if r['part'] in ('kerbalEVA', 'kerbalEVAfemale', 'flag', 'kerbalEVASlimSuit', 'kerbalEVASlimSuitFemale'):
        continue
    stats['rows'] += 1
    diff = [n for n in ('type', 'value', 'isPersistant') if c[n] != r[n]]
    if r['value_num'] != c['value_num'] and not (r['value_num'] is None and c['value_num'] is None):
        diff.append('value_num')
    if diff:
        stats['diff'] += 1
        groups[(cls[(r['part'], r['module_index'])], r['field'], tuple(diff))].append((r['part'], r['value'], c['value']))
print(dict(stats), 'groups', len(groups))
for (cl, f, d), v in sorted(groups.items(), key=lambda x: -len(x[1])):
    p, a, b = v[0]
    print('%4d %s.%s %s  e.g. %s: capture=%r candidate=%r' % (len(v), cl, f, ','.join(d), p, a[:60], b[:60]))
