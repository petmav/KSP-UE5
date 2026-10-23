import sys, glob, collections, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mu_wheel_reader as R
props = collections.Counter(); wraps = collections.Counter(); types = collections.Counter()
for f in sorted(glob.glob(sys.argv[1] + '/*/*.mu')):
    ver, root = R.read_wheel_mu(open(f, 'rb').read())
    for n, _ in R.walk(root):
        for a in n.anims:
            print(os.path.basename(os.path.dirname(f)), n.name, [(c['name'], c['wrap'], max(k[0] for cu in c['curves'] for k in cu['keys'])) for c in a['clips']], repr(a['default']), a['playAutomatically'])
            for c in a['clips']:
                for cu in c['curves']:
                    props[cu['prop']] += 1; wraps[(cu['pre'], cu['post'])] += 1; types[cu['type']] += 1
print(dict(props)); print(dict(wraps)); print(dict(types))
